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

#include <glib.h>
#include <dbus/dbus.h>
#include "gdbus.h"
#include "connman-manager.h"
#include "dbus.h"
#include "session.h"

static DBusConnection *connection;

static int watch_id;

static char *session_path;

struct session_settings {
	char *state;
};

static struct session_settings settings;

static void connman_appeared(DBusConnection *conn, void *user_data)
{
	pold_connman_manager_create_session();
}

static void connman_disappeared(DBusConnection *conn,
		void *user_data)
{
}

void pold_session_set_path(const char *path)
{
	g_free(session_path);
	session_path = g_strdup(path);
}

void pold_session_set_state(const char *state)
{
	g_free(settings.state);
	settings.state = g_strdup(state);
}

void pold_session_init(DBusConnection *conn)
{
	connection = conn;
	watch_id = g_dbus_add_service_watch(conn, CONNMAN_BUS_NAME,
			connman_appeared, connman_disappeared, NULL, NULL);
	settings.state = NULL;
}

void pold_session_final(void)
{
	g_free(settings.state);
	g_dbus_remove_watch(connection, watch_id);
}
