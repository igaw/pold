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

#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <config.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gdbus.h>
#include <dbus/dbus.h>
#include <jansson.h>
#include "log.h"
#include "policy.h"
#include "pold-manager.h"
#include "dbus-common.h"
#include "http-client.h"
#include "dbus-json.h"

/*
 * This file contains data structures and functions related to the
 * administration of policies. All policies are stored in one global
 * policy directory, which is defined in the header file. Only files
 * in the policy directory which end with ".policy" are
 * considered.
 */

/*
 * Struct that represents an agent application from pold's point of view
 */
struct pold_agent_app {
	/*
	 * The id is of the format "agent_owner/app_owner", where agent_owner
	 * is the application's agent's unique D-Bus owner and app_owner is
	 * the application's unique D-Bus owner.
	 */
	char *id;

	/*
	 * A list of policy ids to which the application potentially matches.
	 * Only one of those policy ids corresponds to the active policy.
	 */
	GSList *policy_ids;

	/*
	 * In order to be able to update the agent when a policy changes,
	 * we need to remember the policy that the agent currently knows about.
	 * The policy is represented as a JSON string.
	 */
	char *agent_policy_json;
};

struct update_policies_cb_data {
	void (*cb)(int error, void *data);
	void *data;
};

static struct pold_policy *default_policy;

static struct pold_policy *own_policy;

/*
 * A regular expression which defines valid policy ids.
 */
static GRegex *valid_policy_ids;

/*
 * Maps the id string to a pold_policy.  Every policy must be contained at
 * least once in one of these hash tables.
 */
static GHashTable *id_to_policy;

/*
 * Maps the policy id to a list of apps
 */
static GHashTable *id_to_apps;

/*
 * Maps an app_id to the corresponding app
 */
static GHashTable *app_id_to_app;

/*
 * Set of all apps that need to be updated with a new policy. When loading or
 * deleting policies, the apps that have to be notified about
 * policy changes are marked by adding them to this set. After an update,
 * the set has to be emptied.
 */
static GHashTable *update_apps;

/*
 * D-Bus connection used by the agent update trigger
 */
static DBusConnection *conn;

static void free_policy(gpointer data)
{
	struct pold_policy *policy = data;

	pold_log_debug("Removing policy %s from memory", policy->id);

	g_free(policy->id);
	g_free(policy->json);
	g_free(policy);
}

static bool is_valid_policy_id(const char *id)
{
	if (!id)
		return false;

	return g_regex_match(valid_policy_ids, id, 0, NULL);
}

static int get_policy_priority(const char *id)
{
	if (!id)
		return -1;

	if (g_str_has_prefix(id, "selinux:"))
		return 2;
	else if (g_str_has_prefix(id, "user:"))
		return 1;
	else if (g_str_has_prefix(id, "group:"))
		return 0;

	return -1;
}

/*
 * An app has a list of policies which potentially match. The active policy
 * among those policies is the one whose type has the highest priority. If
 * there are several policies which have the same highest priority type, the
 * first one in the list wins.
 */
static struct pold_policy *get_active_policy(struct pold_agent_app *app)
{
	struct pold_policy *policy = NULL;
	struct pold_policy *current_policy;
	int current_priority;
	GSList *ids;
	char *id;
	int max_priority = -1;

	for (ids = app->policy_ids; ids; ids = ids->next) {
		id = ids->data;

		current_policy = g_hash_table_lookup(id_to_policy, id);
		current_priority = get_policy_priority(id);

		if (current_priority > max_priority && current_policy) {
			policy = current_policy;
			max_priority = current_priority;
		}
	}

	if (!policy)
		policy = default_policy;

	return policy;
}

/*
 * Compares the currently active policy to the the policy that the agent knows
 * about. If they differ, the application will be marked for update.
 */
static void mark_update_apps(void)
{
	struct pold_agent_app *app;
	struct pold_policy *policy;
	GHashTableIter iter;
	void *key, *value;

	g_hash_table_remove_all(update_apps);

	g_hash_table_iter_init (&iter, app_id_to_app);
	while (g_hash_table_iter_next(&iter, &key, &value)) {
		app = (struct pold_agent_app *) value;
		policy = get_active_policy(app);

		if (g_strcmp0(app->agent_policy_json, policy->json) != 0)
			g_hash_table_add(update_apps, app);
	}
}

static struct pold_policy *load_policy(const char *full_path)
{
	struct pold_policy *policy = NULL;
	json_t *root, *id;

	root = json_load_file(full_path, 0, NULL);
	if (!root)
		goto out;
	if (!json_is_object(root))
		goto out;

	id = json_object_get(root, "Id");
	if (!json_is_string(id))
		goto out;

	policy = g_new0(struct pold_policy, 1);
	policy->id = g_strdup(json_string_value(id));
	policy->json = json_dumps(root, 0);

out:
	json_decref(root);

	return policy;
}

static bool is_valid_policy_filename(char *filename)
{
	return g_str_has_suffix(filename, ".policy");
}

/*
 * Loads all policy files which reside in the policy directory
 */
static int load_policies(const char *policy_dir)
{
	GError *g_error;
	int error = 0;
	char *full_path;
	const char *name;
	GDir *dir;
	struct pold_policy *policy;

	pold_log_debug("Loading policies from directory %s",
			policy_dir);

	dir = g_dir_open(policy_dir, 0, &g_error);
	if (!dir) {
		error = g_error->code;
		g_free(g_error);
		pold_log_error("Failed to open directory %s", policy_dir);
		return -error;
	}

	g_hash_table_remove_all(id_to_policy);

	while ((name = g_dir_read_name(dir))) {
		full_path = g_strdup_printf("%s/%s", policy_dir, name);

		if (is_valid_policy_filename(full_path)) {
			policy = load_policy(full_path);

			if (!policy) {
				pold_log_error("Failed to load policy file %s",
						full_path);
				error = -ENOENT;
				goto out;
			}

			g_hash_table_replace(id_to_policy,
					g_strdup(policy->id), policy);
		}

		g_free(full_path);
	}

out:
	g_dir_close(dir);
	return error;
}

static int save_policy(const char *policy_dir, int counter,
		const char *policy_json)
{
	int error = 0;
	char *full_path;

	full_path = g_strdup_printf("%s/%d.policy", policy_dir, counter);
	if (!g_file_set_contents(full_path, policy_json, -1, NULL))
		error = -EIO;
	g_free(full_path);

	return error;
}

static int save_policies(const char *policy_dir, const char *policies_json)
{
	int error = 0;
	json_t *root, *policy_json;
	unsigned int i;

	root = json_loads(policies_json, 0, NULL);

	if (!json_is_array(root)) {
		error = -EINVAL;
		pold_log_error("Policies are not in valid JSON format");
		goto out;
	}

	for (i = 0; i < json_array_size(root); i++) {
		policy_json = json_array_get(root, i);
		if (!json_is_object(policy_json)) {
			error = EINVAL;
			goto out;
		}

		error = save_policy(policy_dir, i, json_dumps(policy_json, 0));
		if (error)
			goto out;
	}

out:
	json_decref(root);
	return error;
}

static int delete_all_policies(const char *policy_dir)
{
	int error = 0;
	GError *g_error;
	char *full_path;
	const char *name;
	GDir *dir;

	dir = g_dir_open(policy_dir, 0, &g_error);
	if (!dir) {
		error = g_error->code;
		g_free(g_error);
		return -error;
	}

	while ((name = g_dir_read_name(dir))) {
		full_path = g_strdup_printf("%s/%s", policy_dir, name);

		if (is_valid_policy_filename(full_path)) {
			error = g_unlink(full_path);
			if (error) {
				error = -EIO;
				goto out;
			}
		}

		g_free(full_path);
	}

out:
	g_dir_close(dir);
	return error;
}

/*
 * Sends the updated policy to all agents who are not yet updated. This
 * only works when the apps have been marked for updates via mark_update_apps.
 * In case of an successful update, we remember the policy string that we
 * sent to the agent in order to tell when the next update is necessary.
 */
static int update_agent_policies(void)
{
	struct pold_policy *policy;
	GHashTableIter iter;
	gpointer key, value;
	int error;
	struct pold_agent_app *app;
	char **agent_and_app_owner;

	g_hash_table_iter_init(&iter, update_apps);
	while (g_hash_table_iter_next(&iter, &key, &value)) {
		app = key;
		policy = get_active_policy(app);

		agent_and_app_owner = g_strsplit(app->id, "/", 2);
		error = pold_manager_update_agent(conn, agent_and_app_owner[0],
				agent_and_app_owner[1], policy);
		g_strfreev(agent_and_app_owner);

		if (error)
			return error;

		g_free(app->agent_policy_json);
		app->agent_policy_json = g_strdup(policy->json);
	}

	return 0;
}

static void free_app(void *pointer)
{
	struct pold_agent_app *app = pointer;

	if (!app)
		return;

	g_free(app->id);
	g_slist_free_full(app->policy_ids, g_free);
	g_free(app->agent_policy_json);
	g_free(app);
}

static int init_default_policy(void)
{
	default_policy = load_policy(DEFAULT_POLICY);
	if (!default_policy) {
		pold_log_error("Failed to load default policy %s",
				DEFAULT_POLICY);
		return -ENOENT;
	}
	return 0;
}

static int init_own_policy(void)
{
	own_policy = load_policy(OWN_POLICY);
	if (!own_policy) {
		pold_log_error("Failed to load pold policy %s",
				OWN_POLICY);
		return -ENOENT;
	}
	return 0;
}

static void free_list(void *list)
{
	g_slist_free(list);
}

static void hashtables_init(void)
{
	id_to_policy = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
			free_policy);
	id_to_apps = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
			free_list);
	app_id_to_app = g_hash_table_new_full(g_str_hash, g_str_equal,
			g_free, free_app);
	update_apps = g_hash_table_new(g_direct_hash, g_direct_equal);
}

/*
 * Note that the policies will be actually freed by free_policies, which
 * is the destroy function for the hash table filename_to_policies.
 */
static void hashtables_final(void)
{
	g_hash_table_destroy(id_to_policy);
	g_hash_table_destroy(id_to_apps);
	g_hash_table_destroy(app_id_to_app);
	g_hash_table_destroy(update_apps);
}

static void stop_watching_app(DBusConnection *connection, void *user_data)
{
	struct pold_agent_app *app = user_data;
	GSList *ids;
	GSList *apps;
	char *id;

	for (ids = app->policy_ids; ids; ids = ids->next) {
		id = ids->data;

		apps = g_hash_table_lookup(id_to_apps, id);
		g_slist_remove(apps, app);
	}

	g_hash_table_remove(app_id_to_app, app->id);
}

static void fill_apps_to_remove(const char *agent_owner,
		GHashTable *apps_to_remove)
{
	GHashTableIter iter;
	void *key, *value;
	struct pold_agent_app *app;
	GSList *apps;

	g_hash_table_iter_init(&iter, id_to_apps);
	while (g_hash_table_iter_next(&iter, &key, &value)) {
		apps = value;

		for (apps = apps->next; apps; apps = apps->next) {
			app = apps->data;

			if (g_str_has_prefix(app->id, agent_owner))
				g_hash_table_add(apps_to_remove, app);
		}
	}
}

static void update_policies_cb(const char *policies_json, void *data)
{
	int error;
	struct update_policies_cb_data *update_policies_cb_data = data;

	if (!policies_json) {
		error = -EINVAL;
		goto out;
	}

	error = delete_all_policies(POLICYDIR);
	if (error)
		goto out;

	error = save_policies(POLICYDIR, policies_json);
	if (error)
		goto out;

	error = load_policies(POLICYDIR);
	if (error)
		goto out;

	mark_update_apps();
	update_agent_policies();

out:
	if (update_policies_cb_data->cb)
		update_policies_cb_data->cb(error, update_policies_cb_data->data);
	g_free(update_policies_cb_data);
}

/*
 * Remove all apps that correspond to a given agent.
 */
void pold_remove_agent_apps(const char *agent_owner)
{
	GHashTable *apps_to_remove;
	GHashTableIter iter;
	void *key, *value;
	struct pold_agent_app *app;

	apps_to_remove = g_hash_table_new(g_direct_hash, g_direct_equal);
	fill_apps_to_remove(agent_owner, apps_to_remove);

	g_hash_table_iter_init(&iter, apps_to_remove);
	while (g_hash_table_iter_next(&iter, &key, &value)) {
		app = value;
		stop_watching_app(NULL, app);
	}

	g_hash_table_destroy(apps_to_remove);
}

static void add_app(const char *id, struct pold_agent_app *app)
{
	GSList *apps;

	apps = g_hash_table_lookup(id_to_apps, id);
	if (!apps) {
		apps = g_slist_append(apps, NULL);
		g_hash_table_insert(id_to_apps, g_strdup(id), apps);
	}

	g_slist_append(apps, app);
}

/*
 * Starts watching an app, which is identified by its agent's D-Bus owner and
 * its own owner. The number of policy id's n_ids which can apply is variable,
 * each id includes the type of the id encoded in the string. E.g., a valid id
 * would be "user:foo".
 */
void pold_policy_watch_app(const char *agent_owner, const char *app_owner,
		int n_ids, ...)
{
	struct pold_agent_app *app;
	const char *policy_id;
	char *app_id;
	va_list ap;
	int i;

	app_id = g_strdup_printf("%s/%s", agent_owner, app_owner);

	if (g_hash_table_contains(app_id_to_app, app_id))
		return;

	app = g_new0(struct pold_agent_app, 1);
	app->id = g_strdup(app_id);
	app->policy_ids = NULL;

	va_start(ap, n_ids);
	for (i = 0; i < n_ids; i++) {
		policy_id = va_arg(ap, const char*);

		if (is_valid_policy_id(policy_id)) {
			app->policy_ids = g_slist_append(app->policy_ids,
					g_strdup(policy_id));
			add_app(policy_id, app);
		} else {
			pold_log_debug(
					"id %s is an unknown "
					"policy type!", policy_id);
		}
	}
	va_end(ap);

	app->agent_policy_json = g_strdup(get_active_policy(app)->json);

	g_hash_table_replace(app_id_to_app, g_strdup(app_id), app);
	g_dbus_add_disconnect_watch(conn, app_owner, stop_watching_app,
			(void *) app, NULL);
	g_free(app_id);
}

void pold_policy_append_to_message(DBusMessage *msg, struct pold_policy *policy)
{
	DBusMessageIter iter;

	dbus_message_iter_init_append(msg, &iter);
	pold_dbus_json_append_string(&iter, policy->json);
}

struct pold_policy *pold_policy_get(const char *policy_id)
{
	return g_hash_table_lookup(id_to_policy, policy_id);
}

struct pold_policy *pold_policy_get_default(void)
{
	return default_policy;
}

struct pold_policy *pold_policy_get_own_policy(void)
{
	return own_policy;
}

struct pold_policy *pold_policy_get_active_policy(const char *app_id)
{
	struct pold_agent_app *agent_app;

	agent_app = g_hash_table_lookup(app_id_to_app, app_id);
	return get_active_policy(agent_app);
}

static void valid_policy_ids_init(void)
{
	valid_policy_ids = g_regex_new("(selinux|user|group):[a-zA-Z]+",
			0, 0, NULL);
}

void pold_policy_update_from_server(void (*cb)(int error, void *data),
		void *data)
{
	struct update_policies_cb_data *update_policies_cb_data;

	update_policies_cb_data = g_new0(struct update_policies_cb_data, 1);
	update_policies_cb_data->cb = cb;
	update_policies_cb_data->data = data;

	pold_http_client_update_policies(update_policies_cb, update_policies_cb_data);
}

int pold_policy_init(DBusConnection *dbus_connection)
{
	int error;

	pold_log_debug("Initializing policy file loading...");
	conn = dbus_connection;
	hashtables_init();
	valid_policy_ids_init();

	error = init_default_policy();
	if (error)
		goto out_default_policy;

	error = init_own_policy();
	if (error)
		goto out_own_policy;

	error = load_policies(POLICYDIR);
	if (error)
		goto out_load_policies;

	return 0;

out_load_policies:
	free_policy(own_policy);
out_own_policy:
	free_policy(default_policy);
out_default_policy:
	g_regex_unref(valid_policy_ids);
	hashtables_final();

	return error;
}

void pold_policy_final(void)
{
	pold_log_debug("Finalizing policy file loading...");
	if (own_policy)
		free_policy(own_policy);
	if (default_policy)
		free_policy(default_policy);
	hashtables_final();
}
