/*
 *
 *  Policy Daemon - pold
 *
 *  Copyright (C) 2014  BWM Car IT GmbH.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <dbus/dbus.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gdbus.h>
#include <jansson.h>
#include "log.h"
#include "dbus-common.h"
#include "fdo-dbus.h"
#include "policy.h"
#include "dbus.h"
#include "pold-manager.h"

#define POLICY_TIMEOUT_IN_SECONDS 10

static DBusConnection *connection;

static const char *pold_unique_bus;

/*
 * Maps the unique D-Bus owner to the object path on which the owner should be
 * notified in case of policy updates.
 */
static GHashTable *object_paths;

/*
 * Data that is needed/filled in during the get_policy_config callback chain.
 */
struct config_data {
	/*
	 * The initial message to which a reply has to be prepared.
	 */
	DBusMessage *pending;

	/*
	 * The D-Bus unique bus name of the agent that corresponds to the app
	 * whose configuration is sought.
	 */
	char *agent_owner;

	/*
	 * The D-Bus unique bus name of the app whose configuration is sought,
	 * needed to retrieve the caller's selinux, user id and group id.
	 */
	char *app_owner;

	/* Needed to identify the policy configuration */
	char *selinux;

	/*
	 * Needed to identify the policy configuration, in case no selinux
	 * is present
	 */
	uid_t uid;

	/*
	 * Needed to identify the policy configuration, in case no selinux
	 * nor user id are present
	 */
	gid_t gid;
};

static void free_config_data(struct config_data *data)
{
	if (data->pending)
		dbus_message_unref(data->pending);
	g_free(data->agent_owner);
	g_free(data->app_owner);
	g_free(data->selinux);
	g_free(data);
}

static const char *get_user(uid_t uid)
{
	struct passwd *pwd;

	pwd = getpwuid(uid);
	if (!pwd) {
		pold_log_error("getpwuid failed with error code %d", errno);
		return NULL;
	}

	return pwd->pw_name;
}

static const char *get_group(gid_t gid)
{
	struct group *group;

	group = getgrgid(gid);
	if (!group) {
		pold_log_error("getgrgid failed with error code %d", errno);
		return NULL;
	}

	return group->gr_name;
}

static int get_groupid(uid_t uid)
{
	struct passwd *pwd;

	pwd = getpwuid(uid);
	if (!pwd) {
		pold_log_error("getpwuid failed with error code %d", errno);
		return -1;
	}

	return pwd->pw_gid;
}

static char *parse_selinux_type(const char *context)
{
	char *ident, **tokens;

	/*
	 * SELinux combines Role-Based Access Control (RBAC), Type
	 * Enforcment (TE) and optionally Multi-Level Security (MLS).
	 *
	 * When SELinux is enabled all processes and files are labeled
	 * with a contex that contains information such as user, role
	 * type (and optionally a level). E.g.
	 *
	 * $ ls -Z
	 * -rwxrwxr-x. wagi wagi unconfined_u:object_r:haifux_exec_t:s0 session_ui.py
	 *
	 * For identifyng application we (ab)using the type
	 * information. In the above example the haifux_exec_t type
	 * will be transfered to haifux_t as defined in the domain
	 * transition and thus we are able to identify the application
	 * as haifux_t.
	 */

	tokens = g_strsplit(context, ":", 0);
	if (g_strv_length(tokens) < 2) {
		g_strfreev(tokens);
		return NULL;
	}

	/* Use the SELinux type as identification token. */
	ident = g_strdup(tokens[2]);

	g_strfreev(tokens);

	return ident;
}

/*
 * Replaces the policy id by a given id if it is empty
 */
static void replace_id_if_empty(struct pold_policy *policy, const char *id)
{
	json_t *json;

	if (strlen(policy->id) == 0) {
		g_free(policy->id);
		policy->id = g_strdup(id);
		json = json_string(policy->json);
		json_object_set(json, "id", json_string(id));
		g_free(policy->json);
		policy->json = json_dumps(json, 0);
	}
}

static void user_cb(unsigned int uid, void *user_data, int err)
{
	DBusMessage *reply;
	struct config_data *data = user_data;
	struct pold_policy *policy;
	char *selinux = NULL, *user = NULL, *group = NULL;
	char *app_id;

	if (err < 0) {
		pold_log_debug("Retrieving user id failed with "
				"error %d", err);
		reply = g_dbus_create_error(data->pending,
				DBUS_ERROR_FAILED, "Retrieving user id failed "
				"with error %d", err);
		if (!reply) {
			pold_log_debug("Could not create D-Bus error reply "
					"message");
			goto out;
		}
		goto out_send_message;
	}

	if (data->selinux)
		selinux = g_strdup_printf("selinux:%s", data->selinux);

	data->uid = uid;
	user = g_strdup_printf("user:%s", get_user(data->uid));

	data->gid = get_groupid(uid);
	group = g_strdup_printf("group:%s", get_group(data->gid));

	pold_log_debug("(selinux, user, group) = (%s, %s, %s)",
			data->selinux, user, group);

	if (selinux)
		pold_policy_watch_app(data->agent_owner, data->app_owner, 3,
				selinux, user, group);
	else
		pold_policy_watch_app(data->agent_owner, data->app_owner, 2,
				user, group);

	app_id = g_strdup_printf("%s/%s", data->agent_owner, data->app_owner);
	policy = pold_policy_get_active_policy(app_id);
	g_free(app_id);

	replace_id_if_empty(policy, user);

	pold_log_debug("Policy for app \"%s\" sent to agent \"%s\":\n%s",
			data->app_owner, data->agent_owner, policy->json);

	reply = dbus_message_new_method_return(data->pending);
	pold_policy_append_to_message(reply, policy);

out_send_message:
	g_dbus_send_message(connection, reply);
out:
	g_free(user);
	g_free(group);
	g_free(selinux);
	free_config_data(data);
}

static void selinux_cb(const unsigned char *selinux, void *user_data,
		int err)
{
	struct config_data *data = user_data;

	if (err < 0) {
		pold_log_debug("Retrieving selinux context failed with "
				"error %d", err);
	} else {
		data->selinux = parse_selinux_type((const char *) selinux);
	}

	pold_fdo_dbus_get_connection_unix_user(connection, data->app_owner,
			user_cb, data);
}

static bool need_http_policy_update(void)
{
	int error;
	GStatBuf stat_buf;
	int current_time, modification_time, delta_time;

	error = g_stat(POLICYDIR, &stat_buf);
	if (error) {
		pold_log_error("The policy directory does not exist");
		return false;
	}

	current_time = time(NULL);
	modification_time = stat_buf.st_mtime;
	delta_time = current_time - modification_time;

	return delta_time > POLICY_TIMEOUT_IN_SECONDS;
}

static void update_from_server_cb(int error, void *user_data)
{
	struct config_data *data;
	DBusMessage *reply;

	data = user_data;

	if (error) {
		pold_log_error("Policy update from server failed");
		reply = g_dbus_create_error(data->pending,
				DBUS_ERROR_FAILED, "Policy update "
				"from server failed");
		if (!reply) {
			pold_log_debug("Could not create D-Bus error reply message");
			goto error;
		}

		g_dbus_send_message(connection, reply);
		goto error;
	}

	pold_log_debug("Policy update from server successful");

	pold_fdo_dbus_get_connection_selinux_context(connection, data->app_owner,
			selinux_cb, data);
	return;

error:
	free_config_data(data);
}

DBusMessage *pold_manager_get_policy_config(DBusConnection *dbus_connection,
		DBusMessage *message, void *user_data)
{
	DBusMessage *reply;
	DBusMessageIter args;
	char *app_owner;
	struct config_data *data;
	struct pold_policy *own_policy;

	data = g_new0(struct config_data, 1);
	data->pending = dbus_message_ref(message);
	data->agent_owner = g_strdup(dbus_message_get_sender(message));

	dbus_message_iter_init(message, &args);
	dbus_message_iter_get_basic(&args, &app_owner);

	data->app_owner = g_strdup(app_owner);

	pold_log_debug("Policy for app \"%s\" requested by agent \"%s\"",
			data->app_owner, data->agent_owner);

	if (g_strcmp0(data->app_owner, pold_unique_bus) == 0) {
		pold_log_debug("pold is being asked about its own policy");

		reply = dbus_message_new_method_return(message);
		if (!reply) {
			pold_log_debug("Could not create D-Bus reply message");
			return NULL;
		}

		own_policy = pold_policy_get_own_policy();
		pold_policy_append_to_message(reply, own_policy);
		g_dbus_send_message(connection, reply);

		pold_log_debug("Policy for app \"%s\" (pold) sent to agent "
				"\"%s\":\n%s", data->app_owner,
				data->agent_owner, own_policy->json);

		free_config_data(data);

		return NULL;
	}

	pold_log_debug("Checking whether policies are up-to-date");

	if (need_http_policy_update()) {
		pold_log_debug("Policies are not up-to-date anymore - update"
				"from server started...");
		pold_policy_update_from_server(update_from_server_cb, data);
	} else {
		pold_log_debug("Policies are still up-to-date, no update from "
				"server needed");
		pold_fdo_dbus_get_connection_selinux_context(connection,
				app_owner, selinux_cb, data);
	}

	return NULL;
}

static void owner_disconnect(DBusConnection *dbus_connection, void *user_data)
{
	char *agent_owner = user_data;

	g_hash_table_remove(object_paths, agent_owner);
	pold_remove_agent_apps(agent_owner);
}

DBusMessage *pold_manager_register_agent(DBusConnection *dbus_connection,
		DBusMessage *message, void *user_data)
{
	DBusMessage *reply;
	const char *agent_owner;
	char *object_path;

	reply = dbus_message_new_method_return(message);
	if (!reply) {
		pold_log_debug("Could not create D-Bus reply message");
		return NULL;
	}

	agent_owner = dbus_message_get_sender(message);

	dbus_message_get_args(message, NULL, DBUS_TYPE_OBJECT_PATH,
				&object_path);

	if (g_hash_table_lookup(object_paths, agent_owner)) {
		pold_log_debug("Agent registration failed, since an "
				"agent is already registered");
		return NULL;
	}

	g_hash_table_replace(object_paths, g_strdup(agent_owner),
				g_strdup(object_path));

	g_dbus_add_disconnect_watch(dbus_connection, agent_owner,
					owner_disconnect, (void *) agent_owner,
					NULL);

	pold_log_debug("Agent %s registered successfully on object "
			"path %s", agent_owner, object_path);

	return reply;
}

DBusMessage *pold_manager_unregister_agent(DBusConnection *dbus_connection,
		DBusMessage *message, void *user_data)
{
	const char *agent_owner;
	char *given_object_path, *stored_object_path;

	agent_owner = dbus_message_get_sender(message);

	dbus_message_get_args(message, NULL, DBUS_TYPE_OBJECT_PATH,
				&given_object_path);

	stored_object_path = g_hash_table_lookup(object_paths, agent_owner);

	if (g_strcmp0(stored_object_path, given_object_path) != 0) {
		pold_log_info("Agent could not be unregistered");
		return g_dbus_create_error(message, DBUS_ERROR_FAILED,
					"Agent could "
					"not be unregistered");
	}

	g_hash_table_remove(object_paths, agent_owner);
	pold_remove_agent_apps(agent_owner);

	pold_log_debug("Agent %s unregistered successfully from "
			"object path %s", agent_owner, given_object_path);

	return g_dbus_create_reply(message, DBUS_TYPE_INVALID);
}

int pold_manager_update_agent(DBusConnection *dbus_connection,
		const char *agent_owner, const char *app_owner,
		struct pold_policy *policy)
{
	DBusMessage *msg = NULL;
	DBusMessageIter msg_iter;
	char *object_path;

	pold_log_debug("Update agents...");

	object_path = g_hash_table_lookup(object_paths, agent_owner);

	pold_log_debug("Update agent %s on object path %s...",
			agent_owner, object_path);

	msg = dbus_message_new_method_call(agent_owner, object_path,
			POLD_AGENT_NOTIFICATION_INTERFACE, "Update");

	if (!msg)
		goto error_nomem;

	dbus_message_iter_init_append(msg, &msg_iter);
	dbus_message_iter_append_basic(&msg_iter, DBUS_TYPE_STRING, &app_owner);

	pold_policy_append_to_message(msg, policy);

	if (!g_dbus_send_message(dbus_connection, msg))
		goto error_nomem;

	return 0;

error_nomem:
	dbus_message_unref(msg);
	pold_log_debug("Agent update failed with error %s",
			strerror(ENOMEM));

	return -ENOMEM;
}

static const GDBusMethodTable manager_methods[] = {
		{ GDBUS_ASYNC_METHOD("GetPolicyConfig", GDBUS_ARGS({"in", "s"}),
				GDBUS_ARGS({"out", "a{sv}"}),
				pold_manager_get_policy_config)
		},
		{ GDBUS_METHOD("RegisterAgent", GDBUS_ARGS({"in", "o"}),
				NULL, pold_manager_register_agent)
		},
		{ GDBUS_METHOD("UnregisterAgent", GDBUS_ARGS({"in", "o"}),
				NULL, pold_manager_unregister_agent)
		},
		{ }
};

bool pold_manager_init(DBusConnection *dbus_connection)
{
	connection = dbus_connection;
	pold_unique_bus = dbus_bus_get_unique_name(dbus_connection);
	object_paths = g_hash_table_new_full(g_str_hash, g_str_equal,
						g_free, g_free);

	if (!g_dbus_register_interface(dbus_connection, POLD_MANAGER_PATH,
			POLD_MANAGER_INTERFACE, manager_methods,
			NULL, NULL, NULL, NULL)) {
		pold_log_error("Error registering manager in D-Bus");
		return false;
	}

	return true;
}

void pold_manager_final(void)
{
	g_hash_table_destroy(object_paths);
}
