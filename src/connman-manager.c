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
#include <dbus/dbus.h>
#include <glib.h>
#include <gdbus.h>
#include "log.h"
#include "dbus-common.h"
#include "dbus.h"
#include "connman-manager.h"
#include "session.h"

static DBusConnection *connection;

static void create_session_cb(DBusPendingCall *call, void *user_data)
{
	DBusMessage *reply;
	DBusMessageIter iter;
	char *session_path;

	reply = dbus_pending_call_steal_reply(call);

	if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR ||
			!dbus_message_has_signature(reply, "o")) {
		pold_log_error("Failed to create session");
		pold_session_set_path(NULL);
		goto out;
	} else {
		pold_log_debug("ConnMan session successfully created!");
	}

	dbus_message_iter_init(reply, &iter);
	dbus_message_iter_get_basic(&iter, &session_path);
	pold_session_set_path(session_path);

out:
	dbus_message_unref(reply);
	dbus_pending_call_unref(call);
}

void pold_connman_manager_create_session(void)
{
	DBusPendingCall *call;
	DBusMessage *message = NULL;
	DBusMessageIter iter, sub_iter, entry, variant, array;
	const char *allowed_bearers = "AllowedBearers";
	const char *ethernet = "ethernet";
	const char *wifi = "wifi";
	const char *internet = "internet";
	const char *notification_path = POLD_NOTIFICATION_PATH;

	message = dbus_message_new_method_call(CONNMAN_BUS_NAME,
			CONNMAN_MANAGER_PATH, CONNMAN_MANAGER_INTERFACE,
			"CreateSession");
	if (!message) {
		pold_log_debug("Can't allocate new message");
		goto out;
	}

	dbus_message_iter_init_append(message, &iter);

	dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}",
			&sub_iter);

	dbus_message_iter_open_container(&sub_iter, DBUS_TYPE_DICT_ENTRY, NULL, &entry);
	dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &allowed_bearers);
	dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "as", &variant);
	dbus_message_iter_open_container(&variant, DBUS_TYPE_ARRAY, "s", &array);
	dbus_message_iter_append_basic(&array, DBUS_TYPE_STRING, &ethernet);
	dbus_message_iter_append_basic(&array, DBUS_TYPE_STRING, &wifi);
	dbus_message_iter_close_container(&variant, &array);
	dbus_message_iter_close_container(&entry, &variant);
	dbus_message_iter_close_container(&sub_iter, &entry);
	dict_append_entry(&sub_iter, "ConnectionType", DBUS_TYPE_STRING,
			&internet);

	dbus_message_iter_close_container(&iter, &sub_iter);

	dbus_message_iter_append_basic(&iter, DBUS_TYPE_OBJECT_PATH,
			&notification_path);

	if (!dbus_connection_send_with_reply(connection, message, &call, -1)) {
		pold_log_debug("Failed to execute method call");
		goto out;
	}

	if (!call) {
		pold_log_debug("D-Bus connection not available");
		goto out;
	}

	dbus_pending_call_set_notify(call, create_session_cb, NULL, NULL);

out:
	dbus_message_unref(message);
}

void pold_connman_manager_init(DBusConnection *conn)
{
	connection = conn;
}
