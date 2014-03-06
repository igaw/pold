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

#ifndef POLICY_H
#define POLICY_H

#include <stdbool.h>
#include <glib.h>
#include <dbus/dbus.h>

#define POLICYDIR STORAGEDIR "/policies"
#define DEFAULT_POLICY STORAGEDIR "/default.policy"
#define OWN_POLICY STORAGEDIR "/pold.policy"

struct pold_policy {
	/*
	 * The id can be either an SELinux, user or group id. The id is of the
	 * format "type:value". Valid types are: "SELinux", "User", "Group".
	 */
	char *id;

	/*
	 * A JSON string that represents the policy.
	 */
	char *json;
};

void pold_remove_agent_apps(const char *agent_owner);

void pold_policy_watch_app(const char *agent_owner, const char *app_owner,
		int n_ids, ...);

struct pold_policy *pold_policy_get(const char *policy_id);

struct pold_policy *pold_policy_get_default(void);

struct pold_policy *pold_policy_get_own_policy(void);

struct pold_policy *pold_policy_get_active_policy(const char *app_id);

void pold_policy_append_to_message(DBusMessage *msg,
		struct pold_policy *policy);

void pold_policy_update_from_server(void (*cb)(int error, void *data),
		void *data);

int pold_policy_init(DBusConnection *dbus_connection);

void pold_policy_final(void);

#endif
