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

#ifndef DBUS_JSON_H
#define DBUS_JSON_H

#include <jansson.h>
#include <dbus/dbus.h>

/*
 * Appends a JSON object to a D-Bus message. The whole
 * object tree is converted, e.g., a json object is converted to a dictionary, a
 * string to a basic D-Bus string, etc.
 */
void pold_dbus_json_append_object(DBusMessageIter *iter, json_t *root);

/*
 * Convenience function for adding a JSON string to a D-Bus message.
 */
void pold_dbus_json_append_string(DBusMessageIter *iter, const char *json);

#endif
