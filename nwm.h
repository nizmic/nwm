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

#ifndef __NWM_H__
#define __NWM_H__

#include <stdbool.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xinerama.h>
#include <glib.h>
#include <libguile.h>

#include "event.h"

typedef struct {
    xcb_connection_t *connection;
    xcb_event_handlers_t event_handlers;
    int default_screen_num;
    xcb_screen_t *screen;
    xcb_key_symbols_t *key_syms;
    bool xinerama_is_active;
    bool stop;
} nwm_t;

extern nwm_t wm_conf;

typedef struct {
    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;
} rect_t;

typedef struct {
    rect_t rect;
    xcb_window_t window;
    uint16_t border_width;
} client_t;

extern GList *client_list;

typedef struct {
    xcb_keysym_t keysym;
    xcb_key_but_mask_t mod_mask;
    SCM scm_proc;
} keybinding_t;

extern GList *keybinding_list;

void update_client_geometry(client_t *);
void map_client(client_t *);
int bind_key(xcb_key_but_mask_t, xcb_keysym_t, SCM);

void border_test(void);
void arrange(void);

#endif
