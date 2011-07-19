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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/select.h>
#include <fcntl.h>
#include <libguile.h>
#include "nwm.h"
#include "repl-server.h"
#include "scheme.h"

typedef struct {
    char buf[BUFSIZE];
    char *begin_p;
    char *end_p;
} io_buffer_t;

typedef struct {
    int sockfd;
    struct sockaddr_un addr;
    io_buffer_t read_buf;
    io_buffer_t write_buf;
    SCM port;
} repl_conn_t;

void io_buffer_reset(io_buffer_t *buf)
{
    buf->begin_p = buf->end_p = buf->buf;
}

io_buffer_t *io_buffer_init(io_buffer_t *buf)
{
    bzero(buf, sizeof(io_buffer_t));
    io_buffer_reset(buf);
    return buf;
}

size_t io_buffer_available(io_buffer_t *buf)
{
    return (&buf->buf[BUFSIZE] - buf->end_p);
}

ssize_t io_buffer_write(io_buffer_t *buf, const char *src, size_t count)
{
    if (io_buffer_available(buf) < count)
        return -1;  /* buffer too full */

    memcpy(buf->end_p, src, count);
    buf->end_p += count;
    return count;
}

repl_conn_t *repl_conn_alloc(void)
{
    return (repl_conn_t *)malloc(sizeof(repl_conn_t));
}

repl_conn_t *repl_conn_init(repl_conn_t *conn)
{
    bzero(conn, sizeof(repl_conn_t));
    io_buffer_reset(&conn->read_buf);
    io_buffer_reset(&conn->write_buf);
    return conn;
}

void repl_conn_free(repl_conn_t *conn)
{
    free(conn);
}

int repl_conn_read(repl_conn_t *conn)
{
    ssize_t nread;
    size_t read_buf_avail = io_buffer_available(&conn->read_buf);
    int retval;
    fprintf(stderr, "buffer has %ld bytes available\n", read_buf_avail);

    if (read_buf_avail > 0) {
        nread = read(conn->sockfd, conn->read_buf.end_p, read_buf_avail);
        retval = nread;
        if (nread < 0) {
            perror("read error");
            retval = -1;
        }
        else if (nread == 0) {
            fprintf(stderr, "EOF for socket %d, removing from "
                    "connection list\n", conn->sockfd);
        }
        else {
            fprintf(stderr, "%ld bytes read\n", nread);
            conn->read_buf.end_p += nread;
            read_buf_avail = io_buffer_available(&conn->read_buf);
            fprintf(stderr, "buffer has %ld bytes available after read\n", read_buf_avail);
        }
    }
    else {
        fprintf(stderr, "read buffer FULL, not calling read()\n");
        retval = -2;
    }
    return retval;
}

int repl_conn_write(repl_conn_t *conn)
{
    fprintf(stderr, "writing reply to socket %d\n", conn->sockfd);

    size_t len = (conn->write_buf.end_p - conn->write_buf.begin_p);
    if (len > 0) {
        fprintf(stderr, "    %ld bytes\n", len);
        /* should be checking this return value and acting accordingly */
        write(conn->sockfd, conn->write_buf.begin_p, len);

        io_buffer_reset(&conn->write_buf);
    }

    return 0;
}


/* static SCM scm_new_port_table_entry(scm_t_bits tag) */
/* { */
/*     SCM port; */
/*     scm_t_port *pt; */

/*     SCM_NEWCELL(port); */
/*     pt = scm_add_to_port_table(port); */
/*     SCM_SET_CELL_TYPE(port, tag); */
/*     SCM_SETPTAB_ENTRY(port, pt); */
/*     return port; */
/* } */

static scm_t_bits io_buffer_port_tag;

static SCM io_buffer_to_port(io_buffer_t *buf)
{
    SCM port;
    scm_t_port *pt;

    port = scm_new_port_table_entry(io_buffer_port_tag);
    SCM_SET_CELL_TYPE(port, io_buffer_port_tag | SCM_OPN | SCM_WRTNG);
    pt = SCM_PTAB_ENTRY(port);
    SCM_SETSTREAM(port, buf);

    return port;
}

static io_buffer_t *port_to_io_buffer(SCM port)
{
    if (!SCM_IMP(port) && (SCM_TYP16(port) == io_buffer_port_tag))
        return (io_buffer_t *)SCM_STREAM(port);
    else
        return NULL;
    scm_wrong_type_arg("port_to_io_buffer", 1, port);
    /* shouldn't get here */
    return NULL;
}

static int io_buffer_port_print(SCM exp, SCM port, scm_print_state *pstate SCM_UNUSED)
{
    scm_puts("#<io_buffer_port>", port);
    return 1;
}

static void io_buffer_port_write(SCM port, const void *data, size_t size)
{
    io_buffer_write(port_to_io_buffer(port),
                    data,
                    size);
}

void init_io_buffer_ports(void)
{
    scm_t_bits tc;

    tc = scm_make_port_type("io-buffer-port", NULL, io_buffer_port_write);
    scm_set_port_print(tc, io_buffer_port_print);

    io_buffer_port_tag = tc;
}

static SCM eval_lisp(void *data)
{
    return scm_c_eval_string((char*)data);
}

static SCM handle_lisp_error(void *data, SCM key, SCM parameters)
{
    fprintf(stderr, "LISP ERROR\n");
    return key;
}

void repl_conn_eval_lisp(repl_conn_t *conn)
{
    scm_set_current_output_port(conn->port);

    SCM write_scm = scm_c_eval_string("write");
 
    size_t lisp_len = (conn->read_buf.end_p - conn->read_buf.begin_p);
    fprintf(stderr, "repl_conn_eval_lisp:\n    lisp_len = %ld\n", lisp_len);
    char *lisp = malloc(lisp_len + 1);
    memcpy(lisp, conn->read_buf.begin_p, lisp_len);
    lisp[lisp_len] = 0;

    SCM res = scm_c_catch(SCM_BOOL_T, /* applies to all exception types */
                          eval_lisp, lisp,
                          handle_lisp_error, NULL,
                          NULL, NULL);

    char *res_str = "\0";
    if (res != SCM_UNSPECIFIED) {
        SCM str_scm = scm_object_to_string(res, write_scm);
        res_str = scm_to_locale_string(str_scm);
    }

    fprintf(stderr, "    result is '%s'\n", res_str);
    
    /* copy result into write buffer - little messy right now*/
    if (res == SCM_UNSPECIFIED) {
        /* write the string including null byte */
        io_buffer_write(&conn->write_buf, res_str, strlen(res_str)+1);
    }
    else {
        /* write the string without null byte */
        io_buffer_write(&conn->write_buf, res_str, strlen(res_str));
        /* write newline and null byte */
        io_buffer_write(&conn->write_buf, "\n", 2);
    }

    io_buffer_reset(&conn->read_buf);
    free(lisp);
}

static void repl_server_socket_init(repl_server_t *server)
{
    fprintf(stderr, "creating socket\n");
    server->sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (server->sockfd < 0) {
        perror("socket creation failed");
        exit(1);
    }
    unlink(SOCKPATH); /* ok if it fails, just making sure it doesn't already exist */
    bzero(&server->addr, sizeof(server->addr));
    server->addr.sun_family = AF_LOCAL;
    strncpy(server->addr.sun_path, SOCKPATH, sizeof(server->addr.sun_path) - 1);
    fprintf(stderr, "binding socket to %s\n", server->addr.sun_path);
    if (bind(server->sockfd, (struct sockaddr *)&server->addr, SUN_LEN(&server->addr)) < 0) {
        perror("bind failed");
        exit(1);
    }

    fprintf(stderr, "setting socket to listen\n");
    listen(server->sockfd, 1);

    /* set socket to non-blocking */
    int val = fcntl(server->sockfd, F_GETFL, 0);
    val = fcntl(server->sockfd, F_SETFL, val | O_NONBLOCK);
    if (val < 0)
        perror("fcntl error");
}

repl_server_t *repl_server_init(void)
{
    repl_server_t *server = (repl_server_t *)malloc(sizeof(repl_server_t));
    repl_server_socket_init(server);
    scm_with_guile(&init_scheme, NULL);
    return server;
}

static void repl_server_accept_conn(repl_server_t *server)
{
    int sockfd;
    struct sockaddr_un addr;
    socklen_t len = sizeof(addr);
    if ((sockfd = accept(server->sockfd, (struct sockaddr *)&addr, &len)) < 0) {
        if (errno != EINTR)
            perror("accept error");
    }
    else {
        fprintf(stderr, "accepted connection\n");
        repl_conn_t *conn = repl_conn_init(repl_conn_alloc());
        conn->sockfd = sockfd;
        memcpy(&conn->addr, &addr, sizeof(struct sockaddr_un));
        conn->port = io_buffer_to_port(&conn->write_buf);
        server->conn_list = g_list_append(server->conn_list, conn);
    }
}

void repl_server_step(repl_server_t *server)
{
    fd_set rset, wset;
    int val, maxfd;
    struct timeval timeout;    

    /* timeout of zero for select because we want no blocking */
    bzero(&timeout, sizeof(timeout));

    FD_ZERO(&rset);
    FD_ZERO(&wset);
    /* monitor server socket for read availability, indicating that there is a
       pending client connection to accept */
    FD_SET(server->sockfd, &rset);

    /* monitor all client connections for read/write availability */
    maxfd = server->sockfd;
    GList *node = server->conn_list;
    while (node) {
        repl_conn_t *conn = (repl_conn_t *)node->data;
        FD_SET(conn->sockfd, &rset);
        FD_SET(conn->sockfd, &wset);
        if (conn->sockfd > maxfd)
            maxfd = conn->sockfd;
        node = node->next;
    }

    val = select(maxfd + 1, &rset, &wset, NULL, &timeout);
    if (val < 0) {
        perror("select error");
        return;
    }

    if (FD_ISSET(server->sockfd, &rset))
        repl_server_accept_conn(server);

    /* Process any client connections that are available for read/write */
    node = server->conn_list;
    GList *delete_list = NULL;
    while (node) {
        repl_conn_t *conn = (repl_conn_t *)node->data;
        ssize_t n = 0;
        if (FD_ISSET(conn->sockfd, &rset)) {
            fprintf(stderr, "read available for socket %d\n", 
                    conn->sockfd);
            if ((n = repl_conn_read(conn)) == 0) {
                fprintf(stderr, "removing connection, socket %d\n", conn->sockfd);
                delete_list = g_list_append(delete_list, conn);
            }
            else if (n > 0) {
                /* we read something - try to evaluate lisp */
                repl_conn_eval_lisp(conn);
            }
        }

        if (FD_ISSET(conn->sockfd, &wset)) {
            if (n > 0) { /* we read something */
                repl_conn_write(conn);
            }
        }
        node = node->next;
    }

    node = delete_list;
    while (node) {
        repl_conn_t *conn = (repl_conn_t *)node->data;
        server->conn_list = g_list_remove(server->conn_list, conn);
        repl_conn_free(conn);
        node = node->next;
    }
    g_list_free(delete_list);
}
