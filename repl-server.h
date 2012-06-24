/* nwm - a programmable window manager
 * Copyright (C) 2010-2012  Nathan Sullivan
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

#include <sys/socket.h>
#include <sys/un.h>

#define BUFSIZE 4096

typedef struct io_buffer {
    char buf[BUFSIZE];
    char *begin_p;
    char *end_p;
} io_buffer_t;

typedef struct repl_conn {
    int sockfd;
    struct sockaddr_un addr;
    io_buffer_t read_buf;
    io_buffer_t write_buf;
    SCM port;
    struct repl_conn *next;
} repl_conn_t;

typedef struct repl_server {
    int sockfd;
    struct sockaddr_un addr;
    repl_conn_t *conn_list;
} repl_server_t;

#define COMPARE_REPL_CONN(x,y) (x->sockfd - y->sockfd)
SGLIB_DEFINE_LIST_PROTOTYPES(repl_conn_t, COMPARE_REPL_CONN, next)

repl_server_t *repl_server_init(void);
void repl_server_step(repl_server_t *server);
void init_io_buffer_ports(void);
void load_init_scheme(void);

#endif
