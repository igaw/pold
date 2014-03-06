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
#include <stdlib.h>
#include <stdio.h>
#include <dbus/dbus.h>
#include <glib.h>
#include <glib-unix.h>
#include <gdbus.h>
#include "log.h"
#include "dbus-common.h"
#include "policy.h"
#include "pold-manager.h"
#include "connman-manager.h"
#include "connman-notification.h"
#include "session.h"
#include "dbus.h"
#include "http-client.h"

#define USERNAME "someuser"
#define PASSWORD "password"

static GMainLoop *loop;

DBusConnection *conn;

static bool debug;

static GOptionEntry entries[] =
{
	{ "debug", 'd', 0, G_OPTION_ARG_NONE, &debug,
		"Send output to the terminal", NULL },
	{ NULL }
};

static gboolean signal_quit(gpointer user_data)
{
	g_main_loop_quit(loop);
	return TRUE;
}

static gboolean signal_update(gpointer user_data)
{
	pold_log_debug("Received SIGUSR2 - policy update from server "
			"triggered");
	pold_policy_update_from_server(NULL, NULL);
	return TRUE;
}

static gint shandlers[4];

static void install_signal_handlers(void)
{
	shandlers[0] = g_unix_signal_add(SIGINT, signal_quit, NULL);
	shandlers[1] = g_unix_signal_add(SIGTERM, signal_quit, NULL);
	shandlers[2] = g_unix_signal_add(SIGHUP, signal_quit, NULL);
	shandlers[3] = g_unix_signal_add(SIGUSR2, signal_update, NULL);
}

static void remove_signal_handlers(void)
{
	unsigned i;

	for (i = 0; i < G_N_ELEMENTS(shandlers); i++)
		g_source_remove(shandlers[i]);
}

static bool init_dbus(void)
{
	if (conn) {
		pold_log_info("Already initialized");
		return true;
	}

	conn = g_dbus_setup_bus(DBUS_BUS_SYSTEM, POLD_BUS_NAME, NULL);
	if (!conn) {
		pold_log_error(
				"Error connecting to the D-Bus daemon");
		return false;
	}

	return true;
}

static bool parse_options(int argc, char *argv[])
{
	GError *error = NULL;
	GOptionContext *context;

	context = g_option_context_new("");
	g_option_context_add_main_entries(context, entries, NULL);

	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		printf("Option parsing failed: %s\n", error->message);
		return false;
	}

	pold_log_set_debug(debug);

	return true;
}

int main(int argc, char *argv[])
{
	int ret = EXIT_SUCCESS;

	if (!parse_options(argc, argv)) {
		ret = EXIT_FAILURE;
		goto out;
	}

	pold_log_info("Starting Policy Daemon");

	if (!init_dbus()) {
		ret = EXIT_FAILURE;
		goto out;
	}

	if (!pold_http_client_init(USERNAME, PASSWORD)) {
		ret = EXIT_FAILURE;
		goto out_http_client;
	}

	loop = g_main_loop_new(NULL, FALSE);
	install_signal_handlers();

	if (pold_policy_init(conn) < 0) {
		ret = EXIT_FAILURE;
		goto out_signal_handlers;
	}

	if (!pold_manager_init(conn)) {
		ret = EXIT_FAILURE;
		goto out_policy;
	}

	if (!pold_connman_notification_init(conn)) {
		ret = EXIT_FAILURE;
		goto out_manager;
	}

	pold_connman_manager_init(conn);
	pold_session_init(conn);

	pold_log_info("Entering main loop");

	g_main_loop_run(loop);

	pold_log_info("Main loop finished");

	pold_session_final();

out_manager:
	pold_manager_final();
out_policy:
	pold_policy_final();
out_signal_handlers:
	remove_signal_handlers();
	g_main_loop_unref(loop);
out_http_client:
	pold_http_client_final();
	dbus_bus_release_name(conn, POLD_BUS_NAME, NULL);
	dbus_connection_unref(conn);
out:
	pold_log_info("Exiting Policy Daemon");

	return ret;
}
