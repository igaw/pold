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

#define POLD_BUS_NAME "de.bmw.pold1"
#define POLD_MANAGER_PATH "/de/bmw/pold1"
#define POLD_MANAGER_INTERFACE "de.bmw.pold.Manager1"
#define POLD_AGENT_NOTIFICATION_INTERFACE "de.bmw.pold.Notification1"
#define POLD_NOTIFICATION_PATH "/de/bmw/pold1"

#define CONNMAN_BUS_NAME "net.connman"
#define CONNMAN_MANAGER_PATH "/"
#define CONNMAN_MANAGER_INTERFACE "net.connman.Manager"
#define CONNMAN_NOTIFICATION_INTERFACE "net.connman.Notification"
