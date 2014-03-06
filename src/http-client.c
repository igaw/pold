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
#include <string.h>
#include <libsoup/soup.h>
#include <glib.h>
#include <glib/gprintf.h>
#include "log.h"
#include "policy.h"
#include "http-client.h"

#define HOST "http://127.0.0.1:9000"
#define UPDATE_URL HOST "/update_policies"

struct soup_session_queue_message_cb_data {
	void (*cb)(const char *policies_json, void *data);
	void *data;
};

static SoupSession *soup_session;

static char *user, *pwd;

static void authenticate_callback(SoupSession *sess, SoupMessage *msg,
		SoupAuth *auth, gboolean retrying, gpointer user_data)
{
	soup_auth_authenticate(auth, user, pwd);
}

bool pold_http_client_init(const char *username, const char *password)
{
	user = g_strdup(username);
	pwd = g_strdup(password);

	soup_session = soup_session_new();
	if (!soup_session)
		return false;

	if (g_signal_connect(soup_session, "authenticate",
			G_CALLBACK(authenticate_callback), NULL) < 1)
		return false;

	return true;
}

void pold_http_client_final(void)
{
	g_free(pwd);
	g_free(user);
}

/*
 * Joins a list of strings to a null-terminated string
 */
static char *join_list(GSList *string_list)
{
	GSList *list = string_list;
	char *ret, *buf;
	int size = 1;

	if (!string_list)
		return NULL;

	for (list = string_list; list; list = list->next) {
		size += strlen(list->data);
	}

	ret = g_new(char, size);
	buf = ret;

	for (list = string_list; list; list = list->next) {
		buf = g_stpcpy(buf, list->data);
	}

	return ret;
}

/*
 * Converts a soup buffer to a null terminated string.
 */
static char *soup_buffer_to_string(SoupBuffer *buf)
{
	char *ret = g_new(char, buf->length + 1);

	ret[buf->length] = 0;
	g_memmove(ret, buf->data, buf->length);

	return ret;
}

static void soup_session_queue_message_cb(SoupSession *sess, SoupMessage *msg,
		void *user_data)
{
	SoupBuffer *buf;
	int offset = 0;
	char *response = NULL;
	GSList *chunks = NULL;
	struct soup_session_queue_message_cb_data *cb_data = user_data;

	pold_log_info("update_callback");

	if (msg->status_code == SOUP_STATUS_OK) {
		while ((buf = soup_message_body_get_chunk(msg->response_body,
				offset))) {
			chunks = g_slist_append(chunks,
					soup_buffer_to_string(buf));
			offset += buf->length;
		}
		response = join_list(chunks);
		g_slist_free_full(chunks, g_free);

		pold_log_debug("Policies received from server:\n%s", response);
	} else {
		pold_log_error("Failed to update policies");
		goto out;
	}

out:
	cb_data->cb(response, cb_data->data);
	g_free(response);
	g_free(cb_data);
}

void pold_http_client_update_policies(void (*cb)
		(const char *policies_json, void *data), void *data)
{
	SoupMessage *msg;
	struct soup_session_queue_message_cb_data *cb_data;

	msg = soup_message_new("GET", UPDATE_URL);

	cb_data = g_new0(struct soup_session_queue_message_cb_data, 1);
	cb_data->cb = cb;
	cb_data->data = data;

	soup_session_queue_message(soup_session, msg, soup_session_queue_message_cb,
			cb_data);
}
