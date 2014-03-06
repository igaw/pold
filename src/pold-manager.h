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

#ifndef POLD_MANAGER_H
#define POLD_MANAGER_H

#include <stdbool.h>
#include <dbus/dbus.h>
#include "policy.h"

DBusMessage *pold_manager_get_policy_config(DBusConnection *dbus_connection,
		DBusMessage *message, void *user_data);

DBusMessage *pold_manager_register_agent(DBusConnection *connection,
		DBusMessage *message, void *user_data);

DBusMessage *pold_manager_unregister_agent(DBusConnection *connection,
		DBusMessage *message, void *user_data);

int pold_manager_update_agent(DBusConnection *dbus_connection,
		const char *agent_owner, const char *app_owner,
		struct pold_policy *policy);

bool pold_manager_init(DBusConnection *dbus_connection);

void pold_manager_final(void);

#endif
