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

#ifndef LOG_H
#define LOG_H

#include <stdbool.h>

#define pold_log_debug(fmt, arg...) do { \
	pold_log_debug_no_prefix("%s:%s() " fmt, \
			__FILE__, __FUNCTION__, ## arg); \
} while (0)

void pold_log_debug_no_prefix(const char *args, ...);

void pold_log_info(const char *args, ...);

void pold_log_error(const char *args, ...);

void pold_log_set_debug(bool debug);

#endif
