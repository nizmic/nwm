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
    int len = sglib_client_t_len(client_list);
    return scm_from_unsigned_integer(len);
}

static SCM scm_all_clients(void)
{
    SCM clients = SCM_EOL;
    SCM smob;
    client_t *client = client_list;
    while (client) {
        SCM_NEWSMOB(smob, client_tag, client);
        clients = scm_append(scm_list_2(clients, scm_list_1(smob)));
        client = client->next;
    }
    return clients;
}

static SCM scm_first_client(void)
{
    SCM client;
    SCM_NEWSMOB(client, client_tag, client_list);
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

    xcb_query_tree_cookie_t c = xcb_query_tree(wm_conf.connection, client->window);
    xcb_query_tree_reply_t *r = xcb_query_tree_reply(wm_conf.connection, c, NULL);

    if ((len = asprintf(&str, "root: %u\nparent: %u\nchildren_len: %u\n",
                        r->root,
                        r->parent,
                        r->children_len)) < 0) {
        fprintf(stderr, "asprintf failed\n");
        return SCM_UNSPECIFIED;
    }
    scm_c_write(out_port, str, len);
    free(str);

    if (r)
        free(r);

    return SCM_UNSPECIFIED;
}

static SCM scm_get_focus_client(void)
{
    xcb_get_input_focus_cookie_t c = xcb_get_input_focus(wm_conf.connection);
    xcb_get_input_focus_reply_t *r = xcb_get_input_focus_reply(wm_conf.connection, c, NULL);
    SCM client;
    if (r) {
        client_t *c = find_client(r->focus);
        if (c) {
            SCM_NEWSMOB(client, client_tag, c);
        }
        free(r);
    }
    return client;
}

static SCM scm_focus_client(SCM client_smob)
{
    client_t *client = (client_t *)SCM_SMOB_DATA(client_smob);
    xcb_void_cookie_t c = xcb_set_input_focus_checked(wm_conf.connection, 
                                                      XCB_INPUT_FOCUS_POINTER_ROOT,
                                                      client->window,
                                                      XCB_CURRENT_TIME);
    xcb_generic_error_t *e = xcb_request_check(wm_conf.connection, c);
    if (e) {
        fprintf(stderr, "xcb_set_input_focus_checked error: %s\n", 
                xcb_event_get_error_label(e->error_code));
        free(e);
    }
    draw_border(client);
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

static SCM scm_border_test(void)
{
    border_test();
    return SCM_UNSPECIFIED;
}

static SCM scm_nwm_log(SCM msg)
{
    char *c_msg = scm_to_locale_string(msg);
    fprintf(stderr, "%s\n", c_msg);
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

    scm_c_define_gsubr("border-test", 0, 0, 0, &scm_border_test);
    scm_c_define_gsubr("get-focus-client", 0, 0, 0, &scm_get_focus_client);
    scm_c_define_gsubr("focus-client", 1, 0, 0, &scm_focus_client);

    scm_c_define_gsubr("log", 1, 0, 0, &scm_nwm_log);

    init_client_type();

    init_io_buffer_ports();

    return NULL;
}
