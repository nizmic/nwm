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

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/time.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xinerama.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_icccm.h>
#include <X11/keysymdef.h>

#define XK_MISCELLANY
#define XK_LATIN1
#include <X11/keysymdef.h>

#include "nwm.h"
#include "scheme.h"
#include "repl-server.h"

typedef void (*event_loop_task_func)(void);

nwm_t wm_conf;

client_t *client_list = NULL;
SGLIB_DEFINE_LIST_FUNCTIONS(client_t, COMPARE_CLIENT, next)

keybinding_t *keybinding_list = NULL;
SGLIB_DEFINE_LIST_FUNCTIONS(keybinding_t, COMPARE_KEYBINDING, next)

void init_wm_conf(void)
{
    memset(&wm_conf, 0, sizeof(nwm_t));
}

client_t *client_alloc(void)
{
    return (client_t *) scm_gc_malloc(sizeof(client_t), "client");
}

client_t *client_init(client_t *client)
{
    memset(client, 0, sizeof(client_t));
    return client;
}

keybinding_t *keybinding_alloc(void)
{
    return (keybinding_t *)malloc(sizeof(keybinding_t));
}

keybinding_t *keybinding_init(keybinding_t *binding)
{
    memset(binding, 0, sizeof(keybinding_t));
    return binding;
}

void print_x_error(xcb_generic_error_t *error)
{
    uint8_t error_code = error->error_code;
    fprintf(stderr, "X error %d: %s\n", error_code, xcb_event_get_error_label(error_code));
}

int handle_startup_error(void *data, xcb_connection_t *c, xcb_generic_error_t *error)
{
    fprintf(stderr, "another window manager is already running\n");
    print_x_error(error);
    exit(1);
}

int handle_error(void *data, xcb_connection_t *c, xcb_generic_error_t *error)
{
    print_x_error(error);
    return 0;
}

void print_x_event(xcb_generic_event_t *event)
{
    uint8_t event_type = XCB_EVENT_RESPONSE_TYPE(event);
    const char *label = xcb_event_get_label(event_type);
    fprintf(stderr, "X event %d : %s\n", event_type, label);
}

client_t * find_client(xcb_window_t win)
{
    client_t *client = client_list;
    while (client) {
        if (client->window == win)
            return client;
        client = client->next;
    }
    return NULL;
}

void draw_border(client_t *client)
{
    /* Clear root window to erase any old borders */
    xcb_aux_clear_window(wm_conf.connection, wm_conf.screen->root);

    xcb_gcontext_t orange = xcb_generate_id(wm_conf.connection);
    uint32_t mask = XCB_GC_FOREGROUND;
    uint32_t value[] = { 0xffffa500 };
    xcb_create_gc(wm_conf.connection, orange, wm_conf.screen->root, mask, value);
    xcb_rectangle_t rect[] = {{ client->rect.x - 2,
                                client->rect.y - 2,
                                client->rect.width + 4,
                                client->rect.height + 4 }};

    /* Draw the new border */
    xcb_poly_fill_rectangle(wm_conf.connection, wm_conf.screen->root, orange, 1, rect);

    xcb_free_gc(wm_conf.connection, orange);
    xcb_map_window(wm_conf.connection, wm_conf.screen->root);
    xcb_flush(wm_conf.connection);
}

int handle_button_press_event(void *data, xcb_connection_t *c, xcb_button_press_event_t *event)
{
    return 0;
}

int handle_button_release_event(void *data, xcb_connection_t *c, xcb_button_press_event_t *event)
{
    return 0;
}

int handle_configure_request_event(void *data, xcb_connection_t *c, xcb_configure_request_event_t *event)
{
    uint16_t config_win_mask = 0;
    uint32_t config_win_vals[5];

    config_win_mask |= (XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | 
                        XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT |
                        XCB_CONFIG_WINDOW_BORDER_WIDTH);

    /* currently assigning full screen size. need to rework this. */
    config_win_vals[0] = 0;
    config_win_vals[1] = 0;
    config_win_vals[2] = wm_conf.screen->width_in_pixels;
    config_win_vals[3] = wm_conf.screen->height_in_pixels;
    config_win_vals[4] = 0;

    xcb_configure_window(c, event->window, config_win_mask, config_win_vals);

    return 0;
}

int handle_configure_notify_event(void *data, xcb_connection_t *c, xcb_configure_notify_event_t *event)
{
    return 0;
}

int handle_destroy_notify_event(void *data, xcb_connection_t *c, xcb_destroy_notify_event_t *event)
{
    client_t *client = find_client(event->window);
    if (client) {
        fprintf(stderr, "destroy notify: removing client window %u\n", client->window);
        sglib_client_t_delete(&client_list, client);
    }
    else {
        fprintf(stderr, "destroy notify: client window %u not found\n", event->window);
    }
    destroy_client(client);
    return 0;
}

int handle_enter_notify_event(void *data, xcb_connection_t *c, xcb_enter_notify_event_t *event)
{
    fprintf(stderr, "enter notify\n");
    return 0;
}

int handle_leave_notify_event(void *data, xcb_connection_t *c, xcb_leave_notify_event_t *event)
{
    fprintf(stderr, "leave notify\n");
    return 0;
}

int handle_focus_in_event(void *data, xcb_connection_t *c, xcb_focus_in_event_t *event)
{
    return 0;
}

int handle_motion_notify_event(void *data, xcb_connection_t *c, xcb_motion_notify_event_t *event)
{
    return 0;
}

int handle_expose_event(void *data, xcb_connection_t *c, xcb_expose_event_t *event)
{
    return 0;
}

void dump_client_list(void)
{
    client_t *client = client_list;
    while (client) {
        fprintf(stderr, "   window %u at (%d, %d) size (%d, %d)\n", 
                client->window,
                client->rect.x,
                client->rect.y,
                client->rect.width,
                client->rect.height);
        client = client->next;
    }
}

int handle_key_press_event(void *data, xcb_connection_t *c, xcb_key_press_event_t *event)
{
    xcb_keycode_t keycode = event->detail;
    xcb_keysym_t keysym = xcb_key_symbols_get_keysym(wm_conf.key_syms, keycode, 0);
    fprintf(stderr, "key press: keycode %u, keysym %u, state %u\n", keycode, keysym, event->state);
    /* search key bindings */
    SCM key_proc = SCM_UNDEFINED;
    keybinding_t *binding = keybinding_list;
    while (binding) {
        if (binding->keysym == keysym && binding->mod_mask == event->state) {
            key_proc = binding->scm_proc;
            break;
        }
        binding = binding->next;
    }

    if (key_proc != SCM_UNDEFINED)
        scm_call(key_proc, SCM_UNDEFINED);

    return 0;
}

int handle_key_release_event(void *data, xcb_connection_t *c, xcb_key_release_event_t *event)
{
    return 0;
}

void map_client(client_t *client)
{
    SCM client_smob = SCM_EOL;
    xcb_map_window(wm_conf.connection, client->window);
    /* the map won't happen immediately unless we flush the connection */
    xcb_flush(wm_conf.connection);
    client_smob = scm_new_smob(client_tag, (scm_t_bits) client);
    run_hook("map-client-hook", scm_list_1(client_smob));
}

void unmap_client(client_t *client)
{
    SCM client_smob;
    xcb_unmap_window(wm_conf.connection, client->window);
    xcb_flush(wm_conf.connection);
    client_smob = scm_new_smob(client_tag, (scm_t_bits) client);
    run_hook("unmap-client-hook", scm_list_1(client_smob));
}

void destroy_client(client_t *client)
{
    SCM client_smob;
    xcb_get_property_cookie_t cookie;
    xcb_icccm_get_wm_protocols_reply_t protocols;
    xcb_atom_t del_win_atom;
    xcb_atom_t win_prot_atom;
    uint32_t i;
    int delete = 0;
    
    del_win_atom = get_atom("WM_DELETE_WINDOW");
    win_prot_atom = get_atom("WM_PROTOCOLS");
    cookie = xcb_icccm_get_wm_protocols_unchecked(wm_conf.connection, client->window,
                                                  win_prot_atom);
    if (xcb_icccm_get_wm_protocols_reply(wm_conf.connection, cookie, &protocols,
                                         NULL) == 1) {
        for (i = 0; i < protocols.atoms_len; i++)
            if (protocols.atoms[i] == del_win_atom)
                delete = 1;
    }
    xcb_icccm_get_wm_protocols_reply_wipe(&protocols);
    if (delete == 1) {
        xcb_client_message_event_t ev = {
            .response_type = XCB_CLIENT_MESSAGE,
            .format = 32,
            .sequence = 0,
            .window = client->window,
            .type = win_prot_atom,
            .data.data32 = { del_win_atom, XCB_CURRENT_TIME }
        };
        xcb_send_event(wm_conf.connection, 0, client->window, XCB_EVENT_MASK_NO_EVENT,
                       (char *) &ev);
    }
    else
        xcb_kill_client(wm_conf.connection, client->window);
    xcb_flush(wm_conf.connection);
    client_smob = scm_new_smob(client_tag, (scm_t_bits) client);
    run_hook("destroy-client-hook", scm_list_1(client_smob));
}

/* Use the geometry data from client structure to configure the X window */
void update_client_geometry(client_t *client)
{
    uint16_t config_win_mask = 0;
    uint32_t config_win_vals[5];

    config_win_mask |= (XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y
                        | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT
                        | XCB_CONFIG_WINDOW_BORDER_WIDTH);

    fprintf(stderr, "updating geometry for window %u to (%d,%d) + (%u,%u), border width=%u\n",
            client->window,
            client->rect.x,
            client->rect.y,
            client->rect.width,
            client->rect.height,
            client->border_width);
    config_win_vals[0] = client->rect.x;
    config_win_vals[1] = client->rect.y;
    config_win_vals[2] = client->rect.width;
    config_win_vals[3] = client->rect.height;
    config_win_vals[4] = client->border_width;

    xcb_configure_window(wm_conf.connection, client->window, config_win_mask, config_win_vals);
}

/* Read the X window geometry and record it in the client structure */
void read_client_geometry(client_t *client)
{
    xcb_get_geometry_cookie_t geometry_cookie;
    xcb_get_geometry_reply_t *geometry_reply;
    geometry_cookie = xcb_get_geometry_unchecked(wm_conf.connection, client->window);
    geometry_reply = xcb_get_geometry_reply(wm_conf.connection, geometry_cookie, NULL);
    if (geometry_reply) {
        client->rect.x = geometry_reply->x;
        client->rect.y = geometry_reply->y;
        client->rect.width = geometry_reply->width;
        client->rect.height = geometry_reply->height;
        client->border_width = geometry_reply->border_width;
        free(geometry_reply);
    }
    else {
        fprintf(stderr, "  ! failed to get geometry geometry for window %u\n", client->window);
    }
}

void get_client_name(client_t *client, char *name_out)
{
    xcb_get_property_cookie_t c;
    xcb_icccm_get_text_property_reply_t *r;
    char *name = "";
    int res;
    int len = 0;

    c = xcb_icccm_get_wm_name(wm_conf.connection, client->window);
    res = xcb_icccm_get_wm_name_reply(wm_conf.connection, c, r, NULL);
    if (res == 1) {
        len = xcb_get_property_value_length((xcb_get_property_reply_t *) r);
        if (len > 0)
            name = (char *)xcb_get_property_value((xcb_get_property_reply_t *) r);
        /* sort of arbitrary name length limit (leaving one byte for
         * the null character) */
        if (len > 255)
            len = 255;
    }
    else
        fprintf(stderr, "failed to get wm_name\n");
    free(r);
    strncpy(name_out, name, len+1);
}

client_t *get_focus_client(void)
{
    xcb_get_input_focus_cookie_t c = xcb_get_input_focus(wm_conf.connection);
    xcb_get_input_focus_reply_t *r = xcb_get_input_focus_reply(wm_conf.connection, c, NULL);
    client_t *focus_client = NULL;

    if (r) {
        focus_client = find_client(r->focus);
        free(r);
    }
    return focus_client;
}

void set_focus_client(client_t *client)
{
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
}

client_t *manage_window(xcb_window_t window)
{
    client_t *client = client_init(client_alloc());
    client->window = window;
    sglib_client_t_add(&client_list, client);

    read_client_geometry(client);
    client->border_width = 0;
    update_client_geometry(client);

    map_client(client);
    return client;    
}

int handle_map_request_event(void *data, xcb_connection_t *c, xcb_map_request_event_t *event)
{
    xcb_get_window_attributes_cookie_t win_attrs_cookie;
    xcb_get_window_attributes_reply_t *win_attrs_reply;
    win_attrs_cookie = xcb_get_window_attributes_unchecked(c, event->window);
    win_attrs_reply = xcb_get_window_attributes_reply(c, win_attrs_cookie, NULL);
    if (!win_attrs_reply) {
        fprintf(stderr, "map request: failed to get window attributes\n");
        return -1;
    }

    if (win_attrs_reply->override_redirect) {
        fprintf(stderr, "map request: window has override redirect set - ignoring map request\n");
        return 0;
    }

    client_t *client = NULL;

    client = find_client(event->window);
    if (!client)
        client = manage_window(event->window);

    map_client(client);

    free(win_attrs_reply);

    return 0;
}

int handle_unmap_notify_event(void *data, xcb_connection_t *c, xcb_unmap_notify_event_t *event)
{
    client_t *client = find_client(event->window);
    if (client && XCB_EVENT_SENT(event)) {
        fprintf(stderr, "unmap notify: unmapping window %u\n", client->window);
        sglib_client_t_delete(&client_list, client);
        unmap_client(client);
        xcb_map_window(wm_conf.connection, event->event);
        free(client);
    }

    /* not right */
    xcb_map_window(wm_conf.connection, event->event);

    return 0;
}

int handle_client_message_event(void *data, xcb_connection_t *c, xcb_client_message_event_t *event)
{
    return 0;
}

/* Mapping notify event is sent to all windows and has to do with keyboard mapping, not
 * window mapping.
 * See http://tronche.com/gui/x/xlib/events/window-state-change/mapping.html
 */
int handle_mapping_notify_event(void *data, xcb_connection_t *c, xcb_mapping_notify_event_t *event)
{
    return 0;
}

int handle_reparent_notify_event(void *data, xcb_connection_t *c, xcb_reparent_notify_event_t *event)
{
    return 0;
}

void set_event_handlers(xcb_event_handlers_t *handlers)
{
    xcb_event_set_button_press_handler(handlers, handle_button_press_event, NULL);
    xcb_event_set_button_release_handler(handlers, handle_button_release_event, NULL);
    xcb_event_set_configure_request_handler(handlers, handle_configure_request_event, NULL);
    xcb_event_set_configure_notify_handler(handlers, handle_configure_notify_event, NULL);
    xcb_event_set_destroy_notify_handler(handlers, handle_destroy_notify_event, NULL);
    xcb_event_set_enter_notify_handler(handlers, handle_enter_notify_event, NULL);
    xcb_event_set_leave_notify_handler(handlers, handle_leave_notify_event, NULL);
    xcb_event_set_focus_in_handler(handlers, handle_focus_in_event, NULL);
    xcb_event_set_motion_notify_handler(handlers, handle_motion_notify_event, NULL);
    xcb_event_set_expose_handler(handlers, handle_expose_event, NULL);
    xcb_event_set_key_press_handler(handlers, handle_key_press_event, NULL);
    xcb_event_set_key_release_handler(handlers, handle_key_release_event, NULL);
    xcb_event_set_map_request_handler(handlers, handle_map_request_event, NULL);
    xcb_event_set_unmap_notify_handler(handlers, handle_unmap_notify_event, NULL);
    xcb_event_set_client_message_handler(handlers, handle_client_message_event, NULL);
    xcb_event_set_mapping_notify_handler(handlers, handle_mapping_notify_event, NULL);
    xcb_event_set_reparent_notify_handler(handlers, handle_reparent_notify_event, NULL);
}

void set_exclusive_error_handler(xcb_event_handlers_t *handlers, xcb_generic_error_handler_t handler)
{
    int i;
    for (i = 0; i < 256; ++i)
        xcb_event_set_error_handler(handlers, i, handler, NULL);
}

xcb_atom_t get_atom(char *atom_name)
{
    xcb_intern_atom_cookie_t atom_cookie;
    xcb_atom_t atom;
    xcb_intern_atom_reply_t *reply;

    atom_cookie = xcb_intern_atom(wm_conf.connection, 0, strlen(atom_name), atom_name);
    reply = xcb_intern_atom_reply(wm_conf.connection, atom_cookie, NULL);
    if (reply != NULL) {
        atom = reply->atom;
        free(reply);
        return atom;
    }

    return 0;
}

void scan_windows(void)
{
    const int screens_len = xcb_setup_roots_length(xcb_get_setup(wm_conf.connection));
    int screen_idx;
    xcb_window_t root_wins[screens_len];
    xcb_query_tree_cookie_t root_tree_cookies[screens_len];

    /* request window trees for the root window of each screen */
    for (screen_idx = 0; screen_idx < screens_len; ++screen_idx) {
        xcb_screen_t *screen = xcb_aux_get_screen(wm_conf.connection, screen_idx);
        root_wins[screen_idx] = screen->root;
        root_tree_cookies[screen_idx] = xcb_query_tree_unchecked(wm_conf.connection,
                                                                 root_wins[screen_idx]);
    }

    xcb_query_tree_reply_t *root_tree_replies[screens_len];
    xcb_window_t *wins;
    int wins_len;

    /* collect replies */
    for (screen_idx = 0; screen_idx < screens_len; ++screen_idx) {
        root_tree_replies[screen_idx] = xcb_query_tree_reply(wm_conf.connection,
                                                             root_tree_cookies[screen_idx],
                                                             NULL);
        if (!root_tree_replies[screen_idx])
            continue;
        wins = xcb_query_tree_children(root_tree_replies[screen_idx]);
        if (!wins)
            fprintf(stderr, "failed to get child tree for window %u\n", root_wins[screen_idx]);
        wins_len = xcb_query_tree_children_length(root_tree_replies[screen_idx]);
        fprintf(stderr, "root window %u has %d children\n", root_wins[screen_idx], wins_len);

        fprintf(stderr, "examining children\n");
        xcb_window_t child_win;
        int i;
        for (i = 0; i < wins_len; ++i) {
            child_win = wins[i];
            fprintf(stderr, "  child window %u\n", child_win);
            client_t *client = find_client(child_win);
            /* TODO: verify that the following commented-out code is
               not needed.  Keeping it causes phantom clients for some
               background processes like dbus and pulseaudio started
               from .xinitrc */

            /* if (!client) { */
            /*     client = client_init(client_alloc()); */
            /*     client->window = child_win; */
            /*     sglib_client_t_add(&client_list, client); */
            /* } */
            if (client)
              read_client_geometry(client);
        }
    }

    for (screen_idx = 0; screen_idx < screens_len; ++screen_idx)
        free(root_tree_replies[screen_idx]);
}

void xinerama_test(void)
{
    /* check for xinerama extension */
    if (xcb_get_extension_data(wm_conf.connection, &xcb_xinerama_id)->present) {
        xcb_xinerama_is_active_reply_t *r;
        r = xcb_xinerama_is_active_reply(wm_conf.connection,
                                         xcb_xinerama_is_active(wm_conf.connection),
                                         NULL);
        wm_conf.xinerama_is_active = r->state;
        free(r);
    }
    fprintf(stderr, "xinerama is %sactive\n", (wm_conf.xinerama_is_active ? "" : "NOT "));

    if (wm_conf.xinerama_is_active) {
        xcb_xinerama_query_screens_reply_t *r;
        xcb_xinerama_screen_info_t *screen_info;
        int num_screens;
        r = xcb_xinerama_query_screens_reply(wm_conf.connection,
                                             xcb_xinerama_query_screens_unchecked(wm_conf.connection),
                                             NULL);
        screen_info = xcb_xinerama_query_screens_screen_info(r);
        num_screens = xcb_xinerama_query_screens_screen_info_length(r);
        fprintf(stderr, "xinerama: num_screens=%d\n", num_screens);
        int screen;
        for (screen = 0; screen < num_screens; ++screen) {
            xcb_xinerama_screen_info_t *info = &screen_info[screen];
            fprintf(stderr, "xinerama: screen %d\n", screen);
            fprintf(stderr, "  x_org = %d\n", info->x_org);
            fprintf(stderr, "  y_org = %d\n", info->y_org);
            fprintf(stderr, "  width = %u\n", info->width);
            fprintf(stderr, "  height = %u\n", info->height);
        }
        free(r);
    }
}

xcb_keysym_t get_keysym(char *key)
{
    xcb_keysym_t keysym = NULL;
    if (strlen(key) == 1)
        keysym = (int)key[0];
    else {
        if (!strcmp(key, "Enter"))
            keysym = XK_Return;
        else if (!strcmp(key, "Backspace"))
            keysym = XK_BackSpace;
        else if (!strcmp(key, "Delete"))
            keysym = XK_Delete;
        else if (!strcmp(key, "Escape"))
            keysym = XK_Escape;
        else if (!strcmp(key, "Tab"))
            keysym = XK_Tab;
        else if (!strcmp(key, "Left"))
            keysym = XK_Left;
        else if (!strcmp(key, "Right"))
            keysym = XK_Right;
        else if (!strcmp(key, "Down"))
            keysym = XK_Down;
        else if (!strcmp(key, "Up"))
            keysym = XK_Up;
        else if (!strcmp(key, "Space"))
            keysym = XK_space;
    }
    return keysym;
}

int bind_key(xcb_key_but_mask_t mod_mask, xcb_keysym_t keysym, SCM proc)
{
    xcb_grab_server(wm_conf.connection);
    xcb_keycode_t *keycode_array = xcb_key_symbols_get_keycode(wm_conf.key_syms,
                                                               keysym);
    xcb_keycode_t keycode;
    int i = 0;
    if (keycode_array) {
        while ((keycode = keycode_array[i++]) != XCB_NO_SYMBOL) {
            xcb_grab_key(wm_conf.connection, 1, wm_conf.screen->root,
                         mod_mask, keycode,
                         XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
        }
        free(keycode_array);
        keybinding_t *binding = keybinding_init(keybinding_alloc());
        binding->keysym = keysym;
        binding->mod_mask = mod_mask;
        binding->scm_proc = proc;
        sglib_keybinding_t_add(&keybinding_list, binding);
    }
    xcb_ungrab_server(wm_conf.connection);
    xcb_flush(wm_conf.connection);
    return 1;
}

void auto_focus_pointer(void)
{
    xcb_query_pointer_cookie_t c = xcb_query_pointer_unchecked(wm_conf.connection,
                                                               wm_conf.screen->root);
    xcb_query_pointer_reply_t *reply = xcb_query_pointer_reply(wm_conf.connection,
                                                               c,
                                                               NULL);
    if (reply) {
        client_t *pointer_client = find_client(reply->child);
        if (!wm_conf.pointer_window)  /* pointer_window hasn't been initialized yet */
            wm_conf.pointer_window = reply->child;
        if (pointer_client) {
            if (wm_conf.pointer_window != pointer_client->window) {
                set_focus_client(pointer_client);
                draw_border(pointer_client);
            }
            wm_conf.pointer_window = pointer_client->window;
            set_focus_client(pointer_client);
        }
        free(reply);
    }
}

void init_conf_dir(void)
{
    char *home_path = getenv("HOME");
    if (!home_path) {
        fprintf(stderr, "HOME is not set. Not initializing config directory.\n");
        return;
    }
    char *conf_dir_path = (char *)malloc((strlen(home_path) + strlen(CONF_DIR) + 2) * sizeof(char));
    strcpy(conf_dir_path, home_path);
    strcat(conf_dir_path, "/");
    strcat(conf_dir_path, CONF_DIR);
    struct stat st;
    int stat_res = stat(conf_dir_path, &st);
    if (stat_res < 0) {
        if (errno == EACCES) {
            fprintf(stderr, "config directory '%s' is not accessible.\n", conf_dir_path);
        }
        else {
            /* Assume directory doesn't exist yet.  Try to create it. */
            fprintf(stderr, "creating config directory '%s'.\n", conf_dir_path);
            if (mkdir(conf_dir_path, 0700)) {
                fprintf(stderr, "failed to create config directory.\n");
            }
        }
    }
    wm_conf.conf_dir_path = conf_dir_path;
}

static void event_task_repl_server(void)
{
    repl_server_step(wm_conf.repl_server);
}

static void event_task_x_events(void)
{
    xcb_generic_event_t *event;
    /* Handle all pending events */
    while ((event = xcb_poll_for_event(wm_conf.connection))) {
        if (wm_conf.trace_x_events)
            print_x_event(event);
        xcb_event_handle(&wm_conf.event_handlers, event);
        free(event);
        xcb_flush(wm_conf.connection);
    }
}

static void event_task_autofocus(void)
{
    auto_focus_pointer();
}

static inline long timeval_usec_diff(struct timeval *begin, struct timeval *end)
{
    return ((end->tv_sec - begin->tv_sec) * 1000000L + 
            (long)(end->tv_usec - begin->tv_usec));
}

static void event_loop(void)
{
    /* Set up a circular list of task functions */
    static const event_loop_task_func task_funcs[] = {
        &event_task_repl_server,
        &event_task_x_events,
        &event_task_autofocus,
    };
    static const int task_func_count = sizeof(task_funcs) / sizeof(event_loop_task_func);
    static const long usec_slice_size = 20000L;

    int task_idx = 0;
    struct timeval begin_time, end_time;
    while (!wm_conf.stop) {
        gettimeofday(&begin_time, NULL);
        gettimeofday(&end_time, NULL);  /* Just making sure end_time is initialized */

        /* Start as many of the tasks (max of once per task) as possible within 
         * usec_slice_size microseconds.  The event loop blocks for the duration
         * of each task.  The loop terminates after either all tasks have been
         * completed or more time has elapsed than usec_slice_size microseconds.
         */
        int i;
        for (i = 0; i < task_func_count; ++i) {
            if (timeval_usec_diff(&begin_time, &end_time) > usec_slice_size)
                break;
            event_loop_task_func task_func = task_funcs[task_idx];
            (*task_func)();
            gettimeofday(&end_time, NULL);
            if (++task_idx == task_func_count)
                task_idx = 0;
        }

        /* Sleep for remaining microseconds of time slice, or 1 microsecond if the
         * entire time slice was used.
         */
        long usleep_time = usec_slice_size - timeval_usec_diff(&begin_time, &end_time);
        usleep(usleep_time > 0 ? usleep_time : 1);
    }
}

int main(int argc, char **argv)
{
    xcb_connection_t *connection = xcb_connect(NULL, &wm_conf.default_screen_num);
    if (xcb_connection_has_error(connection)) {
        fprintf(stderr, "failed to open display\n");
        exit(1);
    }

    init_wm_conf();
    init_conf_dir();
    wm_conf.connection = connection;

    const xcb_setup_t *setup = xcb_get_setup(connection);
    int num_screens = xcb_setup_roots_length(setup);
    fprintf(stderr, "init: num_screens = %d\n", num_screens);
    fprintf(stderr, "init: default_screen_num = %d\n", wm_conf.default_screen_num);

    xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;
    wm_conf.screen = screen;
    xcb_window_t root_window = screen->root;

    xcb_grab_server(connection);
    xcb_flush(connection);

    xcb_event_handlers_t *event_handlers = &wm_conf.event_handlers;
    xcb_event_handlers_init(connection, event_handlers);
    set_exclusive_error_handler(event_handlers, handle_startup_error);
    /* Try to get substructure redirect events from root window.
     * This will cause an error if a window manager is already running.
     */
    const uint32_t sub_redirect = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT;
    xcb_change_window_attributes(connection,
                                 root_window,
                                 XCB_CW_EVENT_MASK,
                                 &sub_redirect);

    /* Need to xcb_flush to validate error handler */
    xcb_aux_sync(connection);

    /* Process all errors in the queue if any */
    xcb_event_poll_for_event_loop(event_handlers);

    scan_windows();

    xinerama_test();

    set_exclusive_error_handler(event_handlers, handle_error);
    set_event_handlers(event_handlers);

    /* Allocate the key symbols */
    wm_conf.key_syms = xcb_key_symbols_alloc(connection);

    fprintf(stderr, "selecting events from root window\n");
    const uint32_t root_win_event_mask = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
        XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
        XCB_EVENT_MASK_ENTER_WINDOW |
        XCB_EVENT_MASK_LEAVE_WINDOW |
        XCB_EVENT_MASK_STRUCTURE_NOTIFY |
        XCB_EVENT_MASK_PROPERTY_CHANGE |
        XCB_EVENT_MASK_BUTTON_PRESS |
        XCB_EVENT_MASK_BUTTON_RELEASE |
        XCB_EVENT_MASK_FOCUS_CHANGE;
    xcb_change_window_attributes(connection, root_window, XCB_CW_EVENT_MASK, &root_win_event_mask);

    xcb_ungrab_server(connection);
    xcb_flush(connection);

    wm_conf.repl_server = repl_server_init();
    load_init_scheme();

    fprintf(stderr, "entering event loop\n");
    event_loop();

    xcb_set_input_focus(connection, XCB_NONE, XCB_INPUT_FOCUS_POINTER_ROOT,
                        XCB_CURRENT_TIME);
    xcb_flush(connection);
    xcb_disconnect(connection);

    return 0;
}
