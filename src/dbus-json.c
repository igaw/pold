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

#include <stdbool.h>
#include <assert.h>
#include <glib.h>
#include <dbus/dbus.h>
#include "dbus-json.h"

/* With jansson v2.5 we have json_array_foreach */
#ifndef json_array_foreach
#define json_array_foreach(array, index, value) \
        for(index = 0; \
                index < json_array_size(array) && (value = json_array_get(array, index)); \
                index++)
#endif

/*
 * Returns the Jansson type of the inner values of the array, if they are all
 * of the same type. Otherwise, -1 is returned.
 */
static int get_array_type(json_t *array)
{
	unsigned int i;
	int type, previous_type;
	json_t *entry;

	if (json_array_size(array) == 0)
		return -1;

	entry = json_array_get(array, 0);
	previous_type = json_typeof(entry);

	type = -1;
	for(i = 1; i < json_array_size(array); i++) {
		entry = json_array_get(array, i);
		type = json_typeof(entry);

		if (type != previous_type)
			return -1;

		previous_type = type;
	}

	return type;
}

static char *json_value_to_dbus_type_signature(json_t *value);

static char *json_array_to_dbus_type_signature(json_t *array)
{
	int type = get_array_type(array);
	char *ret = g_new(char, 3);
	char *inner_type;

	ret[0] = 'a';
	ret[2] = 0;

	switch (type) {
	case JSON_STRING:
	case JSON_INTEGER:
	case JSON_REAL:
	case JSON_TRUE:
	case JSON_FALSE:
	case JSON_NULL:
		inner_type = json_value_to_dbus_type_signature(
				json_array_get(array, 0));
		ret[1] = *inner_type;
		g_free(inner_type);
		break;
	default:
		ret[1] = 'v';
		break;
	}

	return ret;
}

static char *json_value_to_dbus_type_signature(json_t *value)
{
	char *ret;

	if (json_is_boolean(value))
		ret = g_strdup("b");
	else if (json_is_integer(value)) {
		if (sizeof(int) == 2)
			ret = g_strdup("n");
		else if (sizeof(int) == 4)
			ret = g_strdup("i");
		else if (sizeof(int) == 8)
			ret = g_strdup("x");
		else
			assert(false /* sizeof(int) must be 2, 4 or 8 */);
	}
	else if (json_is_real(value))
		ret = g_strdup("d");
	else if (json_is_string(value))
		ret = g_strdup("s");
	else if (json_is_array(value))
		ret = json_array_to_dbus_type_signature(value);
	else if (json_is_object(value))
		ret = g_strdup("a{sv}");
	else
		assert(false /* Unsupported JSON type */);

	return ret;
}

static void append_boolean(DBusMessageIter *iter, json_t *json)
{
	dbus_bool_t boolean = json_is_true(json);

	dbus_message_iter_append_basic(iter, DBUS_TYPE_BOOLEAN, &boolean);
}

static void append_integer(DBusMessageIter *iter, json_t *json)
{
	int integer = json_integer_value(json);
	int type;

	if (sizeof(int) == 2)
		type = DBUS_TYPE_INT16;
	else if (sizeof(int) == 4)
		type = DBUS_TYPE_INT32;
	else if (sizeof(int) == 8)
		type = DBUS_TYPE_INT64;
	else
		assert(false /* sizeof(int) must be 2, 4 or 8 */);

	dbus_message_iter_append_basic(iter, type, &integer);
}

static void append_real(DBusMessageIter *iter, json_t *json)
{
	double real = json_real_value(json);

	dbus_message_iter_append_basic(iter, DBUS_TYPE_DOUBLE, &real);
}

static void append_string(DBusMessageIter *iter, json_t *json)
{
	const char *string = json_string_value(json);

	dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &string);
}

static void append_json_value(DBusMessageIter *iter, json_t *root,
		char *dbus_type_signature);

/*
 * Appends an array to the current D-Bus message iterator. If the array is
 * inhomogeneous (not every entry has the same type), each value is packed
 * in a variant.
 */
static void append_array(DBusMessageIter *iter, json_t *json,
		char *dbus_type_signature)
{
	DBusMessageIter array, variant;
	unsigned int index;
	json_t *json_array_entry;
	char *dbus_type;

	if (!dbus_type_signature)
		dbus_type = json_value_to_dbus_type_signature(json);
	else
		dbus_type = g_strdup(dbus_type_signature);

	dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY,
			&(dbus_type[1]), &array);

	json_array_foreach(json, index, json_array_entry) {
		if (dbus_type[1] == 'v') {
			char *entry_dbus_type_signature;

			entry_dbus_type_signature =
					json_value_to_dbus_type_signature(
							json_array_entry);
			dbus_message_iter_open_container(&array,
					DBUS_TYPE_VARIANT,
					entry_dbus_type_signature, &variant);
			append_json_value(&variant, json_array_entry,
					entry_dbus_type_signature);
			dbus_message_iter_close_container(&array, &variant);
			g_free(entry_dbus_type_signature);
		} else {
			pold_dbus_json_append_object(&array, json_array_entry);
		}
	}

	dbus_message_iter_close_container(iter, &array);

	g_free(dbus_type);
}

static void append_object(DBusMessageIter *iter, json_t *json)
{
	DBusMessageIter dict, dict_entry, variant;
	json_t *value;
	const char *key;
	char *dbus_type_signature;

	dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, "{sv}", &dict);

	json_object_foreach(json, key, value) {
		dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY,
				NULL, &dict_entry);

		dbus_message_iter_append_basic(&dict_entry, DBUS_TYPE_STRING, &key);

		dbus_type_signature = json_value_to_dbus_type_signature(value);

		dbus_message_iter_open_container(&dict_entry,
				DBUS_TYPE_VARIANT, dbus_type_signature, &variant);
		append_json_value(&variant, value, dbus_type_signature);
		dbus_message_iter_close_container(&dict_entry, &variant);

		g_free(dbus_type_signature);

		dbus_message_iter_close_container(&dict, &dict_entry);
	}

	dbus_message_iter_close_container(iter, &dict);
}

/*
 * The type signature is the D-Bus type signature string of the given JSON
 * value. If unknown, NULL can be passed, which causes append_json_value
 * to generate the type signature on its own.
 */
static void append_json_value(DBusMessageIter *iter, json_t *value,
		char *dbus_type_signature)
{
	char *type_signature;

	if (dbus_type_signature)
		type_signature = g_strdup(dbus_type_signature);
	else
		type_signature = json_value_to_dbus_type_signature(value);

	if (json_is_boolean(value))
		append_boolean(iter, value);
	else if (json_is_integer(value))
		append_integer(iter, value);
	else if (json_is_real(value))
		append_real(iter, value);
	else if (json_is_string(value))
		append_string(iter, value);
	else if (json_is_array(value))
		append_array(iter, value, type_signature);
	else if (json_is_object(value))
		append_object(iter, value);
	else
		assert(false /* Unsupported JSON type */);

	g_free(type_signature);
}

void pold_dbus_json_append_object(DBusMessageIter *iter, json_t *root)
{
	append_json_value(iter, root, NULL);
}

void pold_dbus_json_append_string(DBusMessageIter *iter, const char *json)
{
	json_t *root;

	root = json_loads(json, 0, NULL);
	if (!root)
		return;

	append_json_value(iter, root, NULL);
}
