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

#define _GNU_SOURCE
#include <stdio.h>

#include <libguile.h>

#include "nwm.h"
#include "repl-server.h"
#include "scheme.h"

scm_t_bits client_tag;

static SCM mark_client(SCM client_smob)
{
    return SCM_BOOL_F;
}

static size_t free_client(SCM client_smob)
{
    return 0;
}

static int print_client(SCM client_smob, SCM port, scm_print_state *pstate)
{
    client_t *client = (client_t *)SCM_SMOB_DATA(client_smob);

    scm_puts("#<client ", port);
    scm_display(scm_from_unsigned_integer(client->window), port);
    scm_puts(">", port);

    /* success */
    return 1;
}

static void init_client_type(void)
{
    client_tag = scm_make_smob_type("client", sizeof(client_t));
    scm_set_smob_mark(client_tag, mark_client);
    scm_set_smob_free(client_tag, free_client);
    scm_set_smob_print(client_tag, print_client);
}

static SCM scm_move_client(SCM client_smob, SCM x, SCM y)
{
    client_t *client = (client_t *)SCM_SMOB_DATA(client_smob);
    client->rect.x = scm_to_int16(x);
    client->rect.y = scm_to_int16(y);
    update_client_geometry(client);
    return SCM_UNSPECIFIED;
}

static SCM scm_resize_client(SCM client_smob, SCM width, SCM height)
{
    client_t *client = (client_t *)SCM_SMOB_DATA(client_smob);
    client->rect.width = scm_to_uint16(width);
    client->rect.height = scm_to_uint16(height);
    update_client_geometry(client);
    return SCM_UNSPECIFIED;
}

static SCM scm_map_client(SCM client_smob)
{
    client_t *client = (client_t *)SCM_SMOB_DATA(client_smob);
    map_client(client);
    return SCM_UNSPECIFIED;
}

static SCM scm_client_x(SCM client_smob)
{
    client_t *client = (client_t *)SCM_SMOB_DATA(client_smob);
    return scm_from_signed_integer(client->rect.x);
}

static SCM scm_client_y(SCM client_smob)
{
    client_t *client = (client_t *)SCM_SMOB_DATA(client_smob);
    return scm_from_signed_integer(client->rect.y);
}

static SCM scm_client_width(SCM client_smob)
{
    client_t *client = (client_t *)SCM_SMOB_DATA(client_smob);
    return scm_from_unsigned_integer(client->rect.width);
}

static SCM scm_client_height(SCM client_smob)
{
    client_t *client = (client_t *)SCM_SMOB_DATA(client_smob);
    return scm_from_unsigned_integer(client->rect.height);
}

static SCM scm_nwm_stop(void)
{
    wm_conf.stop = true;
    return SCM_BOOL_T;
}

static SCM scm_count_clients(void)
{
    guint len = g_list_length(client_list);
    return scm_from_unsigned_integer(len);
}

static SCM scm_all_clients(void)
{
    SCM clients = SCM_EOL;
    SCM smob;
    GList *node = client_list;
    while (node) {
        SCM_NEWSMOB(smob, client_tag, (client_t *)node->data);
        clients = scm_append(scm_list_2(clients, scm_list_1(smob)));
        node = node->next;
    }
    return clients;
}

static SCM scm_first_client(void)
{
    SCM client;
    SCM_NEWSMOB(client, client_tag, (client_t *)client_list->data);
    return client;
}

static SCM scm_test_undefined(void)
{
    return SCM_EOL;
}

static SCM scm_dump_client(SCM client_smob)
{
    client_t *client = (client_t *)SCM_SMOB_DATA(client_smob);

    SCM out_port = scm_current_output_port();

    char *str = NULL;
    int len;
    const char *fmt = "window: %u\nposition: (%d, %d)\nsize: %u x %u\nborder width: %u\n";
    if ((len = asprintf(&str, fmt, 
                        client->window,
                        client->rect.x, client->rect.y,
                        client->rect.width, client->rect.height,
                        client->border_width)) < 0) {
        fprintf(stderr, "asprintf failed\n");
        /* not sure what to return here, will figure it out later */
        return SCM_UNSPECIFIED;
    }

    scm_c_write(out_port, str, len);

    free(str);

    return SCM_UNSPECIFIED;
}

static SCM scm_screen_width(void)
{
    return scm_from_unsigned_integer(wm_conf.screen->width_in_pixels);
}

static SCM scm_screen_height(void)
{
    return scm_from_unsigned_integer(wm_conf.screen->height_in_pixels);
}

static SCM scm_bind_key(SCM mod_mask, SCM keysym, SCM proc)
{
    bind_key(scm_to_uint16(mod_mask), scm_to_uint32(keysym), proc);
    return SCM_UNSPECIFIED;
}

void *init_scheme(void *data)
{
    scm_c_define_gsubr("nwm-stop", 0, 0, 0, &scm_nwm_stop);
    scm_c_define_gsubr("count-clients", 0, 0, 0, &scm_count_clients);

    scm_c_define_gsubr("all-clients", 0, 0, 0, &scm_all_clients);
    scm_c_define_gsubr("first-client", 0, 0, 0, &scm_first_client);
    scm_c_define_gsubr("test-undefined", 0, 0, 0, &scm_test_undefined);

    scm_c_define_gsubr("move-client", 3, 0, 0, &scm_move_client);
    scm_c_define_gsubr("resize-client", 3, 0, 0, &scm_resize_client);
    scm_c_define_gsubr("map-client", 1, 0, 0, &scm_map_client);
    scm_c_define_gsubr("dump-client", 1, 0, 0, &scm_dump_client);
    scm_c_define_gsubr("client-x", 1, 0, 0, &scm_client_x);
    scm_c_define_gsubr("client-y", 1, 0, 0, &scm_client_y);
    scm_c_define_gsubr("client-width", 1, 0, 0, &scm_client_width);
    scm_c_define_gsubr("client-height", 1, 0, 0, &scm_client_height);

    scm_c_define_gsubr("screen-width", 0, 0, 0, &scm_screen_width);
    scm_c_define_gsubr("screen-height", 0, 0, 0, &scm_screen_height);

    scm_c_define_gsubr("bind-key", 3, 0, 0, &scm_bind_key);

    init_client_type();

    init_io_buffer_ports();

    return NULL;
}
