/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2006-2010  Nokia Corporation
 *  Copyright (C) 2004-2010  Marcel Holtmann <marcel@holtmann.org>
 *  Copyright (C) 2005-2007  Johan Hedberg <johan.hedberg@nokia.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdint.h>

#include <glib.h>
#include <dbus/dbus.h>
#include <gdbus/gdbus.h>

#include "dbus-common.h"

static void append_variant(DBusMessageIter *iter, int type, void *val)
{
	DBusMessageIter value;
	char sig[2] = { type, '\0' };

	dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT, sig, &value);
	dbus_message_iter_append_basic(&value, type, val);
	dbus_message_iter_close_container(iter, &value);
}

static void append_variant_array(DBusMessageIter *iter, int type, void *val,
							int n_elements)
{
	DBusMessageIter variant, array;
	char type_sig[2] = { type, '\0' };
	char array_sig[3] = { DBUS_TYPE_ARRAY, type, '\0' };

	dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT,
						array_sig, &variant);
	dbus_message_iter_open_container(&variant, DBUS_TYPE_ARRAY,
						type_sig, &array);

	if (dbus_type_is_fixed(type) == TRUE) {
		dbus_message_iter_append_fixed_array(&array, type, val,
							n_elements);
	} else if (type == DBUS_TYPE_STRING || type == DBUS_TYPE_OBJECT_PATH) {
		const char ***str_array = val;
		int i;

		for (i = 0; i < n_elements; i++)
			dbus_message_iter_append_basic(&array, type,
							&((*str_array)[i]));
	}

	dbus_message_iter_close_container(&variant, &array);
	dbus_message_iter_close_container(iter, &variant);
}

static void append_variant_list(DBusMessageIter *iter, int type, GSList *list)
{
	DBusMessageIter variant, array;
	const char type_sig[2] = { type, '\0' };
	const char array_type_sig[3] = { DBUS_TYPE_ARRAY, type, '\0' };
	GSList *list_iter = list;

	dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT, array_type_sig, &variant);
	dbus_message_iter_open_container(&variant, DBUS_TYPE_ARRAY, type_sig, &array);

	for (list_iter = list; list_iter; list_iter = list_iter->next)
		dbus_message_iter_append_basic(&array, type, &list_iter->data);

	dbus_message_iter_close_container(&variant, &array);
	dbus_message_iter_close_container(iter, &variant);
}

void dict_append_entry(DBusMessageIter *dict,
			const char *key, int type, void *val)
{
	DBusMessageIter entry;

	if (type == DBUS_TYPE_STRING) {
		const char *str = *((const char **) val);
		if (str == NULL)
			return;
	}

	dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY,
							NULL, &entry);

	dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);
	append_variant(&entry, type, val);

	dbus_message_iter_close_container(dict, &entry);
}

void dict_append_array(DBusMessageIter *dict, const char *key, int type,
			void *val, int n_elements)
{
	DBusMessageIter entry;

	dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY,
						NULL, &entry);

	dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);
	append_variant_array(&entry, type, val, n_elements);

	dbus_message_iter_close_container(dict, &entry);
}

void dict_append_list(DBusMessageIter *dict, const char *key, int type,
		GSList *list)
{
	DBusMessageIter entry;

	dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY, NULL, &entry);

	dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);
	append_variant_list(&entry, DBUS_TYPE_STRING, list);

	dbus_message_iter_close_container(dict, &entry);
}
