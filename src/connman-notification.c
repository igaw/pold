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
#include <glib.h>
#include <gdbus.h>
#include "log.h"
#include "connman-manager.h"
#include "connman-notification.h"
#include "dbus.h"
#include "session.h"

static DBusMessage *release(DBusConnection *conn, DBusMessage *message,
		void *user_data)
{
	DBusMessage *reply;

	pold_session_set_path(NULL);
	reply = dbus_message_new_method_return(message);

	if (!reply) {
		pold_log_debug("Could not create D-Bus reply message");
		return NULL;
	}

	return reply;
}

static DBusMessage *update(DBusConnection *conn, DBusMessage *message,
		void *user_data)
{
	DBusMessage *reply;
	DBusMessageIter dict, entry, value;
	const char *key;
	const char *state;

	reply = dbus_message_new_method_return(message);

	if (!reply) {
		pold_log_debug("Could not create D-Bus reply message");
		return NULL;
	}

	dbus_message_iter_init(message, &dict);

	while (dbus_message_iter_get_arg_type(&dict) == DBUS_TYPE_DICT_ENTRY) {
		dbus_message_iter_recurse(&dict, &entry);
		dbus_message_iter_get_basic(&entry, &key);
		dbus_message_iter_next(&entry);
		dbus_message_iter_recurse(&entry, &value);

		if (g_strcmp0(key, "State") == 0) {
			dbus_message_iter_get_basic(&value, &state);
			pold_session_set_state(state);
		}

		dbus_message_iter_next(&dict);
	}

	return reply;
}

static const GDBusMethodTable notification_methods[] = {
	{	GDBUS_METHOD("Release", NULL,
				NULL, release)
	},
	{	GDBUS_METHOD("Update", GDBUS_ARGS({"in", "a{sv}"}),
				NULL, update)
	},
	{}
};

bool pold_connman_notification_init(DBusConnection *conn)
{
	if (!g_dbus_register_interface(conn, POLD_NOTIFICATION_PATH,
			CONNMAN_NOTIFICATION_INTERFACE, notification_methods,
			NULL, NULL, NULL, NULL)) {
		pold_log_error("Error registering connman notification "
						"interface");
		return false;
	}

	return true;
}
