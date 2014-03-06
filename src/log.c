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

#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include "log.h"

#if (HAVE_SYSTEMD_JOURNAL > 0)
#include <systemd/sd-journal.h>
#else
#include <syslog.h>
#endif

/*
 * If false, the log appears in the systemd/syslog journal, otherwise it is
 * written to the terminal.
 */
static bool log_on_console;
static bool log_debug;

static void log_x(int priority, const char *args, va_list args2)
{
	if (log_on_console) {
		vprintf(args, args2);
		printf("\n");
	} else {
#if (HAVE_SYSTEMD_JOURNAL > 0)
		sd_journal_printv(priority, args, args2);
#else
		vsyslog(priority, args, args2);
#endif
	}
}

void pold_log_debug_no_prefix(const char *args, ...)
{
	va_list args2;

	if (!log_debug)
		return;

	va_start(args2, args);
	log_x(LOG_DEBUG, args, args2);
	va_end(args2);
}

void pold_log_info(const char *args, ...)
{
	va_list args2;

	va_start(args2, args);
	log_x(LOG_INFO, args, args2);
	va_end(args2);
}

void pold_log_error(const char *args, ...)
{
	va_list args2;

	va_start(args2, args);
	log_x(LOG_ERR, args, args2);
	va_end(args2);
}

void pold_log_set_debug(bool debug)
{
	log_on_console = debug;
	log_debug = debug;
}
