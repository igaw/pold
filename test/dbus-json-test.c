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
#include <glib.h>
#include <jansson.h>
#include "../src/dbus-json.h"

char test_json_homogeneous_array[] = "[3, 5]";

char test_json_homogeneous_array_within_object[] =
		"{\"a homogeneous array\": [\"one\", \"two\"] }";

char test_json_inhomogeneous_array[] = "[3, \"foo\"]";

char test_json_array_of_arrays[] = "[[1, 2], [3, 4]]";

char test_json_object[] = "{\"a number\": 3, \"a string\": \"foo\"}";

char test_json_complex_object[] =
		"{\"an array\": [\"one\", 2, 3.3], \"a string\": \"foo\"}";

static DBusMessage *message;

static DBusMessageIter iter;

static json_t *value;

static void init(void)
{
	message = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	dbus_message_iter_init_append(message, &iter);
}

static void final(void)
{
	dbus_message_unref(message);
	json_decref(value);
}

static void test_true(void)
{
	dbus_bool_t result;

	init();

	value = json_true();
	pold_dbus_json_append_object(&iter, value);

	dbus_message_iter_init(message, &iter);
	dbus_message_iter_get_basic(&iter, &result);

	g_assert(result == true);

	final();
}

static void test_false(void)
{
	dbus_bool_t result;

	init();

	value = json_false();
	pold_dbus_json_append_object(&iter, value);

	dbus_message_iter_init(message, &iter);
	dbus_message_iter_get_basic(&iter, &result);

	g_assert(result == false);

	final();
}

static void test_integer(void)
{
	int result;

	init();

	value = json_integer(3);
	pold_dbus_json_append_object(&iter, value);

	dbus_message_iter_init(message, &iter);
	dbus_message_iter_get_basic(&iter, &result);

	g_assert(result == 3);

	final();
}

static void test_real(void)
{
	double result;

	init();

	value = json_real(3.14159265);
	pold_dbus_json_append_object(&iter, value);

	dbus_message_iter_init(message, &iter);
	dbus_message_iter_get_basic(&iter, &result);

	g_assert(result > 3.1415926);
	g_assert(result < 3.1415927);

	final();
}

static void test_string(void)
{
	char *result;

	init();

	value = json_string("foo bar");
	pold_dbus_json_append_object(&iter, value);

	dbus_message_iter_init(message, &iter);
	dbus_message_iter_get_basic(&iter, &result);

	g_assert(g_strcmp0(result, "foo bar") == 0);

	final();
}

static void test_homogeneous_array(void)
{
	DBusMessageIter array;
	int first_entry;
	int second_entry;

	init();

	pold_dbus_json_append_string(&iter, test_json_homogeneous_array);

	dbus_message_iter_init(message, &iter);
	dbus_message_iter_recurse(&iter, &array);

	dbus_message_iter_get_basic(&array, &first_entry);
	dbus_message_iter_next(&array);
	dbus_message_iter_get_basic(&array, &second_entry);

	g_assert(first_entry == 3);
	g_assert(second_entry == 5);

	final();
}

/*
 * This test is necessary, since instead of "av", the D-Bus signature has to
 * be "as", which is not covered by the previous test.
 */
static void test_homogeneous_array_within_object(void)
{
	DBusMessageIter array, dict_entry, variant;
	const char *key1, *entry11, *entry12;

	init();

	pold_dbus_json_append_string(&iter, test_json_homogeneous_array_within_object);

	/*
	 * Read key "a homogeneous array"
	 */
	dbus_message_iter_init(message, &iter);
	dbus_message_iter_recurse(&iter, &array);
	dbus_message_iter_recurse(&array, &dict_entry);
	dbus_message_iter_get_basic(&dict_entry, &key1);

	/*
	 * Read value ["one", "two"]
	 */
	dbus_message_iter_next(&dict_entry);
	dbus_message_iter_recurse(&dict_entry, &variant);
	dbus_message_iter_recurse(&variant, &array);
	dbus_message_iter_get_basic(&array, &entry11);
	dbus_message_iter_next(&array);
	dbus_message_iter_get_basic(&array, &entry12);

	g_assert(g_strcmp0(key1, "a homogeneous array") == 0);
	g_assert(g_strcmp0(entry11, "one") == 0);
	g_assert(g_strcmp0(entry12, "two") == 0);

	final();
}

static void test_inhomogeneous_array(void)
{
	DBusMessageIter array, variant;
	int first_entry;
	const char *second_entry;

	init();

	pold_dbus_json_append_string(&iter, test_json_inhomogeneous_array);

	dbus_message_iter_init(message, &iter);
	dbus_message_iter_recurse(&iter, &array);

	dbus_message_iter_recurse(&array, &variant);
	dbus_message_iter_get_basic(&variant, &first_entry);

	dbus_message_iter_next(&array);
	dbus_message_iter_recurse(&array, &variant);
	dbus_message_iter_get_basic(&variant, &second_entry);

	g_assert(first_entry == 3);
	g_assert(g_strcmp0(second_entry, "foo") == 0);

	final();
}

static void test_array_of_arrays(void)
{
	DBusMessageIter array1, array2, variant;
	int entry11, entry12, entry21, entry22;

	init();

	pold_dbus_json_append_string(&iter, test_json_array_of_arrays);

	dbus_message_iter_init(message, &iter);
	dbus_message_iter_recurse(&iter, &array1);
	dbus_message_iter_recurse(&array1, &variant);
	dbus_message_iter_recurse(&variant, &array2);
	dbus_message_iter_get_basic(&array2, &entry11);
	dbus_message_iter_next(&array2);
	dbus_message_iter_get_basic(&array2, &entry12);
	dbus_message_iter_next(&array1);

	/*
	 * Second array
	 */
	dbus_message_iter_recurse(&array1, &variant);
	dbus_message_iter_recurse(&variant, &array2);
	dbus_message_iter_get_basic(&array2, &entry21);
	dbus_message_iter_next(&array2);
	dbus_message_iter_get_basic(&array2, &entry22);

	g_assert(entry11 == 1);
	g_assert(entry12 == 2);
	g_assert(entry21 == 3);
	g_assert(entry22 == 4);

	final();
}

static void test_object(void)
{
	DBusMessageIter array, dict_entry, variant;
	int first_entry;
	const char *key1, *key2, *second_entry;

	init();

	pold_dbus_json_append_string(&iter, test_json_object);

	dbus_message_iter_init(message, &iter);
	dbus_message_iter_recurse(&iter, &array);

	dbus_message_iter_recurse(&array, &dict_entry);

	dbus_message_iter_get_basic(&dict_entry, &key1);

	dbus_message_iter_next(&dict_entry);
	dbus_message_iter_recurse(&dict_entry, &variant);
	dbus_message_iter_get_basic(&variant, &first_entry);

	dbus_message_iter_next(&array);

	dbus_message_iter_recurse(&array, &dict_entry);

	dbus_message_iter_get_basic(&dict_entry, &key2);

	dbus_message_iter_next(&dict_entry);
	dbus_message_iter_recurse(&dict_entry, &variant);
	dbus_message_iter_get_basic(&variant, &second_entry);

	g_assert(g_strcmp0(key1, "a number") == 0);
	g_assert(g_strcmp0(key2, "a string") == 0);
	g_assert(first_entry == 3);
	g_assert(g_strcmp0(second_entry, "foo") == 0);

	final();
}

static void test_complex_object(void)
{
	DBusMessageIter array, array2, dict_entry, variant;
	const char *key1, *key2, *entry11, *entry2;
	int entry12;
	double entry13;

	init();

	pold_dbus_json_append_string(&iter, test_json_complex_object);

	/*
	 * Read key "an array"
	 */
	dbus_message_iter_init(message, &iter);
	dbus_message_iter_recurse(&iter, &array);
	dbus_message_iter_recurse(&array, &dict_entry);
	dbus_message_iter_get_basic(&dict_entry, &key1);

	/*
	 * Read value ["one", 2, 3.3]
	 */
	dbus_message_iter_next(&dict_entry);
	dbus_message_iter_recurse(&dict_entry, &variant);
	dbus_message_iter_recurse(&variant, &array2);
	dbus_message_iter_recurse(&array2, &variant);
	dbus_message_iter_get_basic(&variant, &entry11);
	dbus_message_iter_next(&array2);
	dbus_message_iter_recurse(&array2, &variant);
	dbus_message_iter_get_basic(&variant, &entry12);
	dbus_message_iter_next(&array2);
	dbus_message_iter_recurse(&array2, &variant);
	dbus_message_iter_get_basic(&variant, &entry13);

	/*
	 * Read key "a string"
	 */
	dbus_message_iter_next(&array);
	dbus_message_iter_recurse(&array, &dict_entry);
	dbus_message_iter_get_basic(&dict_entry, &key2);

	/*
	 * Read value "foo"
	 */
	dbus_message_iter_next(&dict_entry);
	dbus_message_iter_recurse(&dict_entry, &variant);
	dbus_message_iter_get_basic(&variant, &entry2);

	g_assert(g_strcmp0(key1, "an array") == 0);
	g_assert(g_strcmp0(entry11, "one") == 0);
	g_assert(entry12 == 2);
	g_assert(entry13 > 3.29);
	g_assert(entry13 < 3.31);
	g_assert(g_strcmp0(key2, "a string") == 0);
	g_assert(g_strcmp0(entry2, "foo") == 0);

	final();
}

int main(int argc, char *argv[])
{
	int error;

	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/dbus-json/test_true", test_true);
	g_test_add_func("/dbus-json/test_false", test_false);
	g_test_add_func("/dbus-json/test_integer", test_integer);
	g_test_add_func("/dbus-json/test_real", test_real);
	g_test_add_func("/dbus-json/test_string", test_string);
	g_test_add_func("/dbus-json/test_homogeneous_array", test_homogeneous_array);
	g_test_add_func("/dbus-json/test_homogeneous_array_within_object",
			test_homogeneous_array_within_object);
	g_test_add_func("/dbus-json/test_inhomogeneous_array", test_inhomogeneous_array);
	g_test_add_func("/dbus-json/test_array_of_arrays", test_array_of_arrays);
	g_test_add_func("/dbus-json/test_object", test_object);
	g_test_add_func("/dbus-json/test_complex_object", test_complex_object);

	error = g_test_run();

	return error;
}
