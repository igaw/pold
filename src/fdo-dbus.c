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
#include <glib.h>
#include "log.h"
#include "fdo-dbus.h"

#define DBUS_UNIQUE_BUSNAME "org.freedesktop.DBus"
#define DBUS_OBJECT_PATH "/"
#define DBUS_INTERFACE "org.freedesktop.DBus"

struct callback_data {
	void *cb;
	void *data;
};

static unsigned char *parse_context(DBusMessage *msg)
{
	DBusMessageIter iter, array;
	unsigned char *ctx, *p;
	int size = 0;

	dbus_message_iter_init(msg, &iter);
	dbus_message_iter_recurse(&iter, &array);
	while (dbus_message_iter_get_arg_type(&array) == DBUS_TYPE_BYTE) {
		size++;

		dbus_message_iter_next(&array);
	}

	if (size == 0)
		return NULL;

	ctx = g_try_malloc0(size + 1);
	if (!ctx)
		return NULL;

	p = ctx;

	dbus_message_iter_init(msg, &iter);
	dbus_message_iter_recurse(&iter, &array);
	while (dbus_message_iter_get_arg_type(&array) == DBUS_TYPE_BYTE) {
		dbus_message_iter_get_basic(&array, p);

		p++;
		dbus_message_iter_next(&array);
	}

	return ctx;
}

static void get_connection_selinux_context_reply(DBusPendingCall *call,
		void *user_data)
{
	struct callback_data *data = user_data;
	pold_dbus_get_connection_selinux_context_cb cb = data->cb;
	DBusMessage *reply;
	unsigned char *context = NULL;
	int err = 0;

	reply = dbus_pending_call_steal_reply(call);

	if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) {
		pold_log_debug("Failed to retrieve SELinux context");
		err = -EIO;
		goto out;
	}

	if (!dbus_message_has_signature(reply, "ay")) {
		pold_log_debug("Message signature is wrong");
		err = -EINVAL;
		goto out;
	}

	context = parse_context(reply);

out:
	(*cb)(context, data->data, err);
	g_free(context);
	dbus_message_unref(reply);
	dbus_pending_call_unref(call);
}

int pold_fdo_dbus_get_connection_selinux_context(DBusConnection *connection,
			const char *service,
			pold_dbus_get_connection_selinux_context_cb callback,
			void *user_data)
{
	struct callback_data *data;
	DBusPendingCall *call;
	DBusMessage *msg = NULL;
	int err;

	if (!callback)
		return -EINVAL;

	data = g_new0(struct callback_data, 1);

	msg = dbus_message_new_method_call(DBUS_UNIQUE_BUSNAME,
					DBUS_OBJECT_PATH, DBUS_INTERFACE,
					"GetConnectionSELinuxSecurityContext");
	if (!msg) {
		pold_log_debug("Can't allocate new message");
		err = -ENOMEM;
		goto error;
	}

	dbus_message_append_args(msg, DBUS_TYPE_STRING, &service,
			DBUS_TYPE_INVALID);

	if (!dbus_connection_send_with_reply(connection, msg, &call, -1)) {
		pold_log_debug("Failed to execute method call");
		err = -EINVAL;
		goto error;
	}

	if (!call) {
		pold_log_debug("D-Bus connection not available");
		err = -EINVAL;
		goto error;
	}

	data->cb = callback;
	data->data = user_data;

	dbus_pending_call_set_notify(call, get_connection_selinux_context_reply,
							data, g_free);

	dbus_message_unref(msg);

	return 0;

error:
	dbus_message_unref(msg);
	g_free(data);

	return err;
}

static void get_connection_unix_user_reply(DBusPendingCall *call,
						void *user_data)
{
	struct callback_data *data = user_data;
	pold_dbus_get_connection_unix_user_cb cb = data->cb;
	DBusMessageIter iter;
	DBusMessage *reply;
	int err = 0;
	unsigned int uid = 0;

	reply = dbus_pending_call_steal_reply(call);

	if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) {
		pold_log_debug("Failed to retrieve UID");
		err = -EIO;
		goto out;
	}

	if (!dbus_message_has_signature(reply, "u")) {
		pold_log_debug("Message signature is wrong");
		err = -EINVAL;
		goto out;
	}

	dbus_message_iter_init(reply, &iter);
	dbus_message_iter_get_basic(&iter, &uid);

out:
	(*cb)(uid, data->data, err);
	dbus_message_unref(reply);
	dbus_pending_call_unref(call);
}

int pold_fdo_dbus_get_connection_unix_user(DBusConnection *connection,
				const char *owner,
				pold_dbus_get_connection_unix_user_cb callback,
				void *user_data)
{
	struct callback_data *data;
	DBusPendingCall *call;
	DBusMessage *msg = NULL;
	int err;

	data = g_new0(struct callback_data, 1);

	msg = dbus_message_new_method_call(DBUS_UNIQUE_BUSNAME,
					DBUS_OBJECT_PATH, DBUS_INTERFACE,
					"GetConnectionUnixUser");
	if (!msg) {
		pold_log_debug("Can't allocate new message");
		err = -ENOMEM;
		goto error;
	}

	dbus_message_append_args(msg, DBUS_TYPE_STRING, &owner,
					DBUS_TYPE_INVALID);

	if (!dbus_connection_send_with_reply(connection, msg, &call, -1)) {
		pold_log_debug("Failed to execute method call");
		err = -EINVAL;
		goto error;
	}

	if (!call) {
		pold_log_debug("D-Bus connection not available");
		err = -EINVAL;
		goto error;
	}

	data->cb = callback;
	data->data = user_data;

	/*
	 * Note that the free function only frees the structure, not the data
	 * itself!
	 */
	dbus_pending_call_set_notify(call, get_connection_unix_user_reply,
			data, g_free);

	dbus_message_unref(msg);

	return 0;

error:
	dbus_message_unref(msg);
	g_free(data);

	return err;
}
