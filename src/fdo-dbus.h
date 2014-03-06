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

#ifndef FDO_DBUS_H
#define FDO_DBUS_H

#include <dbus/dbus.h>

typedef void (*pold_dbus_get_connection_selinux_context_cb) (
		const unsigned char *context, void *user_data, int err);

int pold_fdo_dbus_get_connection_selinux_context(DBusConnection *connection,
		const char *owner,
		pold_dbus_get_connection_selinux_context_cb callback,
		void *user_data);

typedef void (*pold_dbus_get_connection_unix_user_cb) (unsigned int uid,
		void *user_data, int err);

int pold_fdo_dbus_get_connection_unix_user(DBusConnection *connection,
				const char *owner,
				pold_dbus_get_connection_unix_user_cb callback,
				void *user_data);

#endif
