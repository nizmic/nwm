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

#ifndef __NWM_H__
#define __NWM_H__

#include <stdbool.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xinerama.h>
#include <libguile.h>

#include "event.h"
#include "sglib.h"

/* Configuration data directory, relative to $HOME */
#define CONF_DIR ".nwm"

typedef struct nwm {
    xcb_connection_t *connection;
    xcb_event_handlers_t event_handlers;
    int default_screen_num;
    xcb_screen_t *screen;
    xcb_key_symbols_t *key_syms;
    bool xinerama_is_active;
    bool stop;
    char *conf_dir_path;
} nwm_t;

extern nwm_t wm_conf;

typedef struct rect {
    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;
} rect_t;

typedef struct client {
    rect_t rect;
    xcb_window_t window;
    uint16_t border_width;
    struct client *next;
} client_t;

extern client_t *client_list;
#define COMPARE_CLIENT(x,y) (x->window - y->window)
SGLIB_DEFINE_LIST_PROTOTYPES(client_t, COMPARE_CLIENT, next)

typedef struct keybinding {
    xcb_keysym_t keysym;
    xcb_key_but_mask_t mod_mask;
    SCM scm_proc;
    struct keybinding *next;
} keybinding_t;

extern keybinding_t *keybinding_list;
#define COMPARE_KEYBINDING(x,y) (x->keysym - y->keysym)
SGLIB_DEFINE_LIST_PROTOTYPES(keybinding_t, COMPARE_KEYBINDING, next)

void update_client_geometry(client_t *);
void map_client(client_t *);
client_t *find_client(xcb_window_t);
int bind_key(xcb_key_but_mask_t, xcb_keysym_t, SCM);

void border_test(void);
void draw_border(client_t *);
void run_arrange_hook(void);

#endif
