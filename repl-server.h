/* nwm - a programmable window manager
 * Copyright (C) 2010  Nathan Sullivan
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License 
 * as published by the Free Software Foundation; either version 2 
 * of the License, or (at your option) any later version. 
 *
 * This program is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * GNU General Public License for more details. 
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  
 * 02110-1301, USA 
 */

#ifndef __REPL_SERVER_H__
#define __REPL_SERVER_H__

#include <glib.h>
#include <sys/socket.h>
#include <sys/un.h>

#define BUFSIZE 4096

typedef struct {
    int sockfd;
    struct sockaddr_un addr;
    GList *conn_list;
} repl_server_t;

repl_server_t *repl_server_init(void);
void repl_server_step(repl_server_t *server);
void init_io_buffer_ports(void);

#endif
