/* nwm - a programmable window manager
 * Copyright (C) 2013 Brandon Invergo
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
#include <errno.h>
#include <unistd.h>

#include <libguile.h>

#include "nwm.h"
#include "repl-server.h"
#include "scheme.h"

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

static SCM equalp_client(SCM client_smob1, SCM client_smob2)
{
    client_t *client1 = (client_t *)SCM_SMOB_DATA(client_smob1);
    client_t *client2 = (client_t *)SCM_SMOB_DATA(client_smob2);

    if (client1->window == client2->window)
        return SCM_BOOL_T;
    return SCM_BOOL_F;
}
    

static void init_client_type(void)
{
    client_tag = scm_make_smob_type("client", sizeof(client_t));
    scm_set_smob_mark(client_tag, mark_client);
    scm_set_smob_free(client_tag, free_client);
    scm_set_smob_print(client_tag, print_client);
    scm_set_smob_equalp(client_tag, equalp_client);
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

static SCM scm_unmap_client(SCM client_smob)
{
    client_t *client = (client_t *)SCM_SMOB_DATA(client_smob);
    unmap_client(client);
    return SCM_UNSPECIFIED;
}

static SCM scm_is_mapped(SCM client_smob)
{
    /* if (scm_equal_p(client_smob, SCM_UNSPECIFIED)) */
    /*     return SCM_BOOL_F; */
    client_t *client = (client_t *)SCM_SMOB_DATA(client_smob);
    if (is_mapped(client))
        return SCM_BOOL_T;
    else
        return SCM_BOOL_F;
}

static SCM scm_destroy_client(SCM client_smob)
{
    client_t *client = (client_t *)SCM_SMOB_DATA(client_smob);
    destroy_client(client);
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

static SCM scm_clear(void)
{
    clear_root();
    return SCM_UNSPECIFIED;
}

static SCM scm_draw_border(SCM client_smob, SCM color, SCM width)
{
    client_t *client = (client_t *)SCM_SMOB_DATA(client_smob);
    uint32_t color_uint;
    int width_int;
    if (scm_is_integer(color))
        color_uint = scm_to_uint32(color);
    else
        color_uint = 0x6CA0A3;
    if (scm_is_integer(width))
        width_int = scm_to_int(width);
    else
        width_int = 1;
    draw_border(client, color_uint, width_int);
    return SCM_UNSPECIFIED;
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

static SCM scm_visible_clients(void)
{
    SCM clients = SCM_EOL;
    SCM smob;
    client_t *client = client_list;
    while (client) {
        if (is_mapped(client)) {
            SCM_NEWSMOB(smob, client_tag, client);
            clients = scm_append(scm_list_2(clients, scm_list_1(smob)));
        }
        client = client->next;
    }
    return clients;
}

static SCM scm_client_list_reverse(void)
{
    sglib_client_t_reverse(&client_list);
    return SCM_UNSPECIFIED;
}

static SCM scm_client_list_swap(SCM client1_smob, SCM client2_smob)
{
    client_t *client1 = (client_t *)SCM_SMOB_DATA(client1_smob);
    client_t *client2 = (client_t *)SCM_SMOB_DATA(client2_smob);
    rect_t temp_rect;
    xcb_window_t temp_window;
    uint16_t temp_border_width;

    temp_rect = client1->rect;
    temp_window = client1->window;
    temp_border_width = client1->border_width;
    client1->rect = client2->rect;
    client1->window = client2->window;
    client1->border_width = client2->border_width;
    client2->rect = temp_rect;
    client2->window = temp_window;
    client2->border_width = temp_border_width;
    
    return SCM_UNSPECIFIED;
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

static SCM scm_get_client_name(SCM client_smob)
{
    client_t *client = NULL;
    /* sort of arbitrary name length limit */
    char name_buf[256];
    SCM scm_name = SCM_UNSPECIFIED;
    if (scm_is_eq(client_smob, SCM_UNSPECIFIED))
        return SCM_UNSPECIFIED;
    client = (client_t *)SCM_SMOB_DATA(client_smob);
    if (!client)
        return SCM_UNSPECIFIED;
    get_client_name(client, name_buf);
    scm_name = scm_from_locale_string(name_buf);
    return scm_name;
}

static SCM scm_get_focus_client(void)
{
    client_t *focus_client = get_focus_client();
    SCM client_smob = SCM_UNSPECIFIED;
    if (focus_client)
        SCM_NEWSMOB(client_smob, client_tag, focus_client);

    return client_smob;
}

static SCM scm_focus_client(SCM client_smob)
{
    client_t *client = NULL;
    if (scm_is_eq(client_smob, SCM_UNSPECIFIED))
        client = client_list;  // Use first client in list if we aren't given a good client_smob
    else
        client = (client_t *)SCM_SMOB_DATA(client_smob);

    if (!client)
        return SCM_UNSPECIFIED;

    if (!is_mapped(client))
        return SCM_UNSPECIFIED;

    set_focus_client(client);
    return SCM_UNSPECIFIED;
}

static SCM scm_next_client(SCM client_smob)
{
    SCM next_client;
    if (scm_is_eq(client_smob, SCM_UNSPECIFIED))
        return client_smob;
    client_t *client = (client_t *)SCM_SMOB_DATA(client_smob);
    if (client->next) {
        SCM_NEWSMOB(next_client, client_tag, client->next);
    }
    else {
        SCM_NEWSMOB(next_client, client_tag, client_list);
    }
    return next_client;
}

static SCM scm_prev_client(SCM client_smob)
{
    SCM prev_client;
    client_t *client = (client_t *)SCM_SMOB_DATA(client_smob);
    client_t *cur = client_list;
    while (cur->next && cur->next != client)
        cur = cur->next;
    SCM_NEWSMOB(prev_client, client_tag, cur);
    return prev_client;
}

static SCM scm_screen_width(void)
{
    return scm_from_unsigned_integer(wm_conf.screen->width_in_pixels);
}

static SCM scm_screen_height(void)
{
    return scm_from_unsigned_integer(wm_conf.screen->height_in_pixels);
}

static SCM scm_bind_key(SCM mod_mask, SCM key, SCM proc)
{
    xcb_keysym_t keysym;
    if (scm_is_true(scm_number_p(key)))
        keysym = scm_to_uint32(key);
    else if (scm_is_true(scm_string_p(key))) {
        scm_dynwind_begin(0);
        char *c_key = scm_to_locale_string(key);
        scm_dynwind_free(c_key);
        keysym = get_keysym(c_key);
        scm_dynwind_end();
    }
    else
        return SCM_UNSPECIFIED;
    bind_key(scm_to_uint16(mod_mask), keysym, proc);
    return SCM_UNSPECIFIED;
}

static SCM scm_nwm_log(SCM msg)
{
    scm_dynwind_begin(0);
    char *c_msg = scm_to_locale_string(msg);
    scm_dynwind_free(c_msg);
    fprintf(stderr, "%s\n", c_msg);
    scm_dynwind_end();
    return SCM_UNSPECIFIED;
}

static SCM scm_launch_program(SCM prog)
{
    scm_dynwind_begin(0);
    char *c_path = scm_to_locale_string(scm_car(prog));
    scm_dynwind_free(c_path);
    fprintf(stderr, "launching program %s\n", c_path);
    pid_t pid = fork();
    if (pid == 0) {
      if (scm_is_false(scm_execlp(scm_car(prog), prog))) {
            perror("execl failed");
            exit(2);
        }
    }
    else {
        fprintf(stderr, "launched %s as pid %d\n", c_path, pid);
    }
    scm_dynwind_end();
    return SCM_UNSPECIFIED;
}

static SCM scm_trace_x_events(SCM status)
{
    if (status == SCM_BOOL_T)
        wm_conf.trace_x_events = 1;
    else if (status == SCM_BOOL_F)
        wm_conf.trace_x_events = 0;
    return (wm_conf.trace_x_events ? SCM_BOOL_T : SCM_BOOL_F);
}

void run_hook(const char *hook_name, SCM args)
{
    SCM hook_symb = scm_from_utf8_symbol(hook_name);
    SCM hook = scm_eval(hook_symb, scm_interaction_environment());
    if (scm_is_false(scm_defined_p(hook_symb, SCM_UNDEFINED))) {
        fprintf(stderr, "error: %s undefined\n", hook_name);
        return;
    }
    else if (scm_is_false(scm_hook_p(hook))) {
        fprintf(stderr, "error: %s is not a hook!\n", hook_name);
        return;
    }
    if (scm_is_false(scm_hook_empty_p(hook)))
        scm_run_hook(hook, args);
}

void *init_scheme(void *data)
{
    scm_c_define_gsubr("nwm-stop", 0, 0, 0, &scm_nwm_stop);
    scm_c_define_gsubr("count-clients", 0, 0, 0, &scm_count_clients);

    scm_c_define_gsubr("all-clients", 0, 0, 0, &scm_all_clients);
    scm_c_define_gsubr("visible-clients", 0, 0, 0, &scm_visible_clients);
    scm_c_define_gsubr("first-client", 0, 0, 0, &scm_first_client);
    scm_c_define_gsubr("next-client", 1, 0, 0, &scm_next_client);
    scm_c_define_gsubr("prev-client", 1, 0, 0, &scm_prev_client);
    scm_c_define_gsubr("client-list-reverse", 0, 0, 0, &scm_client_list_reverse);
    scm_c_define_gsubr("client-list-swap", 2, 0, 0, &scm_client_list_swap);

    scm_c_define_gsubr("test-undefined", 0, 0, 0, &scm_test_undefined);

    scm_c_define_gsubr("move-client", 3, 0, 0, &scm_move_client);
    scm_c_define_gsubr("resize-client", 3, 0, 0, &scm_resize_client);
    scm_c_define_gsubr("map-client", 1, 0, 0, &scm_map_client);
    scm_c_define_gsubr("unmap-client", 1, 0, 0, &scm_unmap_client);
    scm_c_define_gsubr("mapped?", 1, 0, 0, &scm_is_mapped);    
    scm_c_define_gsubr("destroy-client", 1, 0, 0, &scm_destroy_client);
    scm_c_define_gsubr("dump-client", 1, 0, 0, &scm_dump_client);
    scm_c_define_gsubr("client-x", 1, 0, 0, &scm_client_x);
    scm_c_define_gsubr("client-y", 1, 0, 0, &scm_client_y);
    scm_c_define_gsubr("client-width", 1, 0, 0, &scm_client_width);
    scm_c_define_gsubr("client-height", 1, 0, 0, &scm_client_height);

    scm_c_define_gsubr("screen-width", 0, 0, 0, &scm_screen_width);
    scm_c_define_gsubr("screen-height", 0, 0, 0, &scm_screen_height);

    scm_c_define_gsubr("bind-key", 3, 0, 0, &scm_bind_key);

    scm_c_define_gsubr("clear", 0, 0, 0, &scm_clear);
    scm_c_define_gsubr("draw-border", 3, 0, 0, &scm_draw_border);
    scm_c_define_gsubr("get-focus-client", 0, 0, 0, &scm_get_focus_client);
    scm_c_define_gsubr("focus-client", 1, 0, 0, &scm_focus_client);
    scm_c_define_gsubr("get-client-name", 1, 0, 0, &scm_get_client_name);

    scm_c_define_gsubr("launch-program", 1, 0, 0, &scm_launch_program);

    scm_c_define_gsubr("log", 1, 0, 0, &scm_nwm_log);
    scm_c_define_gsubr("trace-x-events", 1, 0, 0, &scm_trace_x_events);

    scm_c_define("map-client-hook", scm_make_hook(scm_from_int(1)));
    scm_c_define("unmap-client-hook", scm_make_hook(scm_from_int(1)));
    scm_c_define("destroy-client-hook", scm_make_hook(scm_from_int(1)));
    scm_c_define("focus-client-hook", scm_make_hook(scm_from_int(1)));
    scm_c_define("update-client-hook", scm_make_hook(scm_from_int(1)));

    init_client_type();

    init_io_buffer_ports();

    return NULL;
}
