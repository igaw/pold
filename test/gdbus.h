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

#ifndef GDBUS_H
#define GDBUS_H

#include <dbus/dbus.h>
#include <glib.h>

typedef void (* GDBusWatchFunction) (DBusConnection *connection,
		void *user_data);

typedef void (* GDBusDestroyFunction) (void *user_data);

guint g_dbus_add_disconnect_watch(DBusConnection *connection, const char *name,
		GDBusWatchFunction function, void *user_data,
		GDBusDestroyFunction destroy);

#endif
