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

#ifndef STORAGEDIR
#define STORAGEDIR ""
#endif

#include <stdbool.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gdbus.h>
#include "../src/policy.c"

char testdir[] = "pold_test_XXXXXX";
char test_policy1[] =
	"{"\
		"\"Id\": \"selinux:abcde\", "\
		"\"RoamingPolicy\": \"forbidden\", "\
		"\"ConnectionType\": \"internet\", "\
		"\"AllowedBearers\": [\"wifi\", \"cellular\"]"\
	"}";
char test_policy2[] = "{\"Id\" : \"user:foouser2\"}";
char test_policy3[] = "{\"Id\" : \"user:foouser\"}";
char test_policy4[] = "{\"Id\" : \"group:bargroup\"}";
char test_default_policy[] = "{\"Id\" : \"default:default\"}";

static struct pold_policy *load_file(const char *filename)
{
	struct pold_policy *policy;
	char *full_path;

	full_path = g_strdup_printf("%s/%s", testdir, filename);
	policy = load_policy(full_path);
	g_free(full_path);

	return policy;
}

static void save_file(const char *policy_json, const char *filename)
{
	char *full_path;

	g_mkdtemp(testdir);
	full_path = g_strdup_printf("%s/%s", testdir, filename);
	g_file_set_contents(full_path, policy_json, strlen(policy_json), NULL);
	g_free(full_path);
}

static void delete_file(const char *filename)
{
	char *full_path;

	full_path = g_strdup_printf("%s/%s", testdir, filename);
	g_unlink(full_path);
	g_free(full_path);
}

static void test_is_valid_policy_id(void)
{
	valid_policy_ids_init();

	g_assert(!is_valid_policy_id(NULL));

	g_assert(is_valid_policy_id("selinux:foo"));
	g_assert(is_valid_policy_id("user:foo"));
	g_assert(is_valid_policy_id("group:foo"));

	g_assert(!is_valid_policy_id("selinuxfoo"));
	g_assert(!is_valid_policy_id("userfoo"));
	g_assert(!is_valid_policy_id("groupfoo"));

	g_assert(!is_valid_policy_id("selinux:"));
	g_assert(!is_valid_policy_id("user:"));
	g_assert(!is_valid_policy_id("group:"));
	g_assert(!is_valid_policy_id("foo:"));

	g_assert(!is_valid_policy_id("foo:bar"));
}

static void test_load_policy(void)
{
	char *full_path;
	struct pold_policy *policy;

	full_path = g_strdup_printf("%s/%s", testdir, "test1.policy");
	policy = load_policy(full_path);
	g_free(full_path);

	g_assert(policy);
	g_assert(g_strcmp0(policy->id, "selinux:abcde") == 0);
	g_assert(g_strcmp0(policy->json, test_policy1) == 0);
}

static void test_pold_watch_app(void)
{
	struct pold_agent_app *app1, *app2, *app3;
	GSList *apps;

	hashtables_init();

	pold_policy_watch_app("", ":1", 3, "selinux:bazselinux", "user:foouser",
			"group:bargroup");
	pold_policy_watch_app("", ":2", 2, "user:foouser", "group:bargroup");
	pold_policy_watch_app("", ":3", 1, "group:bargroup");
	pold_policy_watch_app("", ":4", 1, "user:baruser");

	g_assert(g_hash_table_size(app_id_to_app) == 4);
	g_assert(g_hash_table_contains(app_id_to_app, "/:1"));
	g_assert(g_hash_table_contains(app_id_to_app, "/:2"));
	g_assert(g_hash_table_contains(app_id_to_app, "/:3"));
	g_assert(g_hash_table_contains(app_id_to_app, "/:4"));

	apps = g_hash_table_lookup(id_to_apps, "selinux:bazselinux");
	g_assert(g_slist_length(apps->next) == 1);
	app1 = apps->next->data;
	g_assert(g_strcmp0(app1->id, "/:1") == 0);

	apps = g_hash_table_lookup(id_to_apps, "user:foouser");
	g_assert(g_slist_length(apps->next) == 2);
	app1 = apps->next->data;
	app2 = apps->next->next->data;
	g_assert(g_strcmp0(app1->id, "/:1") == 0);
	g_assert(g_strcmp0(app2->id, "/:2") == 0);

	apps = g_hash_table_lookup(id_to_apps, "user:baruser");
	g_assert(g_slist_length(apps->next) == 1);
	app1 = apps->next->data;
	g_assert(g_strcmp0(app1->id, "/:4") == 0);

	apps = g_hash_table_lookup(id_to_apps, "group:bargroup");
	g_assert(g_slist_length(apps->next) == 3);
	app1 = apps->next->data;
	app2 = apps->next->next->data;
	app3 = apps->next->next->next->data;
	g_assert(g_strcmp0(app1->id, "/:1") == 0);
	g_assert(g_strcmp0(app2->id, "/:2") == 0);
	g_assert(g_strcmp0(app3->id, "/:3") == 0);
}

static void test_pold_watch_app_twice(void)
{
	struct pold_agent_app *app1;
	GSList *apps;

	hashtables_init();

	pold_policy_watch_app("", ":1", 1, "user:foouser");
	pold_policy_watch_app("", ":1", 1, "user:foouser");

	g_assert(g_hash_table_size(app_id_to_app) == 1);
	g_assert(g_hash_table_contains(app_id_to_app, "/:1"));

	apps = g_hash_table_lookup(id_to_apps, "user:foouser");
	g_assert(g_slist_length(apps->next) == 1);
	app1 = apps->next->data;
	g_assert(g_strcmp0(app1->id, "/:1") == 0);
}

/*
 * Check that invalid ids are ignored
 */
static void test_pold_watch_app_invalid_id(void)
{
	hashtables_init();
	valid_policy_ids_init();

	pold_policy_watch_app("", ":1", 3, "user:foouser",
		"foo:bar", "bar:foo");

	g_assert(g_hash_table_contains(id_to_apps, "user:foouser"));
	g_assert(!g_hash_table_contains(id_to_apps, "foo:bar"));
	g_assert(!g_hash_table_contains(id_to_apps, "bar:foo"));
}

static void test_pold_stop_watching_app(void)
{
	struct pold_agent_app *app1, *app2;
	GSList *apps;

	hashtables_init();

	pold_policy_watch_app("", ":1", 3, "selinux:bazselinux", "user:foouser",
			"group:bargroup");
	pold_policy_watch_app("", ":2", 2, "user:foouser", "group:bargroup");
	pold_policy_watch_app("", ":3", 1, "group:bargroup");
	pold_policy_watch_app("", ":4", 1, "user:baruser");

	apps = g_hash_table_lookup(id_to_apps, "selinux:bazselinux");
	g_assert(g_slist_length(apps->next) == 1);
	app1 = apps->next->data;
	g_assert(g_strcmp0("/:1", app1->id) == 0);

	stop_watching_app(NULL, app1);

	apps = g_hash_table_lookup(id_to_apps, "selinux:bazselinux");
	g_assert(g_slist_length(apps->next) == 0);

	apps = g_hash_table_lookup(id_to_apps, "user:foouser");
	g_assert(g_slist_length(apps->next) == 1);
	app1 = apps->next->data;
	g_assert(g_strcmp0(app1->id, "/:2") == 0);

	apps = g_hash_table_lookup(id_to_apps, "user:baruser");
	g_assert(g_slist_length(apps->next) == 1);
	app1 = apps->next->data;
	g_assert(g_strcmp0(app1->id, "/:4") == 0);

	apps = g_hash_table_lookup(id_to_apps, "group:bargroup");
	g_assert(g_slist_length(apps->next) == 2);
	app1 = apps->next->data;
	app2 = apps->next->next->data;
	g_assert(g_strcmp0(app1->id, "/:2") == 0);
	g_assert(g_strcmp0(app2->id, "/:3") == 0);
}

/*
 * Check that a user type policy wins over a group type policy, and
 * that if there are two user type policies, the first one wins.
 */
static void test_get_active_policy(void)
{
	struct pold_policy *policy;
	struct pold_agent_app *app;

	hashtables_init();
	load_policies(testdir);

	app = g_new0(struct pold_agent_app, 1);
	app->policy_ids = g_slist_append(app->policy_ids,
			g_strdup("group:bargroup"));
	app->policy_ids = g_slist_append(app->policy_ids,
			g_strdup("user:foouser2"));
	app->policy_ids = g_slist_append(app->policy_ids,
			g_strdup("user:foouser"));

	policy = get_active_policy(app);

	g_assert(g_strcmp0(policy->id, "user:foouser2") == 0);
}

static void test_mark_update_apps(void)
{
	struct pold_policy *policy3, *policy4;
	struct pold_agent_app *app;
	GSList *apps;

	hashtables_init();

	/* Load policy 4 */
	policy4 = load_file("test4.policy");
	g_hash_table_replace(id_to_policy, g_strdup(policy4->id), policy4);

	pold_policy_watch_app("agent foo", ":1", 3, "selinux:fooselinux",
			"user:foouser", "group:bargroup");
	apps = g_hash_table_lookup(id_to_apps, "user:foouser");
	app = (struct pold_agent_app *) g_slist_last(apps)->data;

	g_assert(get_active_policy(app) == policy4);

	/* No update needed until now */
	g_assert(g_hash_table_size(update_apps) == 0);
	mark_update_apps();
	g_assert(g_hash_table_size(update_apps) == 0);

	/* Load policy 3 */
	policy3 = load_file("test3.policy");
	g_hash_table_replace(id_to_policy, g_strdup(policy3->id), policy3);

	g_assert(get_active_policy(app) == policy3);

	/* Now the app has to be updated */
	mark_update_apps();
	g_assert(g_hash_table_size(update_apps) == 1);
	g_assert(g_hash_table_lookup(update_apps, app) == app);

	/* Out */
	hashtables_final();
}

static void test_pold_remove_agent_apps(void)
{
	hashtables_init();

	pold_policy_watch_app("agent foo", ":1", 3, "selinux:selinux1",
			"user:foouser", "bar:bargroup");
	pold_policy_watch_app("agent bar", ":2", 3, "selinux:selinux2",
			"user:foouser", "group:bargroup");
	pold_policy_watch_app("agent foo", ":3", 3, "selinux:selinux3",
			"user:baruser", "group:bargroup");

	g_assert(g_hash_table_size(app_id_to_app) == 3);
	g_assert(g_hash_table_contains(app_id_to_app, "agent foo/:1"));
	g_assert(g_hash_table_contains(app_id_to_app, "agent bar/:2"));
	g_assert(g_hash_table_contains(app_id_to_app, "agent foo/:3"));

	pold_remove_agent_apps("agent foo");

	g_assert(g_hash_table_size(app_id_to_app) == 1);
	g_assert(g_hash_table_contains(app_id_to_app, "agent bar/:2"));
}

int main(int argc, char *argv[])
{
	int err;

	save_file(test_policy1, "test1.policy");
	save_file(test_policy2, "test2.policy");
	save_file(test_policy3, "test3.policy");
	save_file(test_policy4, "test4.policy");
	save_file(test_default_policy, "default.policy");

	default_policy = load_file("default.policy");

	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/policy/is_valid_policy", test_is_valid_policy_id);
	g_test_add_func("/policy/load_policy", test_load_policy);
	g_test_add_func("/policy/pold_policy_watch_app", test_pold_watch_app);
	g_test_add_func("/policy/pold_policy_watch_app_twice", test_pold_watch_app_twice);
	g_test_add_func("/policy/pold_policy_watch_app_invalid_id",
			test_pold_watch_app_invalid_id);
	g_test_add_func("/policy/pold_stop_watching_app",
			test_pold_stop_watching_app);
	g_test_add_func("/policy/pold_get_active_policy",
			test_get_active_policy);
	g_test_add_func("/policy/mark_udpate_apps",
			test_mark_update_apps);
	g_test_add_func("/policy/pold_remove_agent_apps",
			test_pold_remove_agent_apps);

	err = g_test_run();

	delete_file("default.policy");
	delete_file("test4.policy");
	delete_file("test3.policy");
	delete_file("test2.policy");
	delete_file("test1.policy");
	g_rmdir(testdir);

	return err;
}
