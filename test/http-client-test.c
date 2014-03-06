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
#include "../src/http-client.c"

static void test_join_list(void)
{
	GSList *list = NULL;
	char *foo;
	char *bar;
	char *joined;

	foo = g_strdup("foo");
	bar = g_strdup("bar");

	list = g_slist_append(list, foo);
	list = g_slist_append(list, bar);
	list = g_slist_append(list, bar);
	list = g_slist_append(list, foo);

	joined = join_list(list);

	g_assert(g_strcmp0(joined, "foobarbarfoo") == 0);

	g_free(bar);
	g_free(foo);
	g_slist_free(list);
	g_free(joined);
}

static void test_soup_buffer_to_string(void)
{
	SoupBuffer buf;
	char *input;
	char *string;

	input = g_strdup("abcdefghijklmn");
	buf.data = input;
	buf.length = 5;

	string = soup_buffer_to_string(&buf);
	g_assert(g_strcmp0(string, "abcde") == 0);

	g_free(string);
	g_free(input);
}

int main(int argc, char *argv[])
{
	int error;

	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/http-client/join_list", test_join_list);
	g_test_add_func("/http-client/soup_buffer_to_string",
			test_soup_buffer_to_string);

	error = g_test_run();

	return error;
}
