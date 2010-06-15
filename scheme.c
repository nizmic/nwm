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

#include <libguile.h>

#include "nwm.h"
#include "scheme.h"

scm_t_bits window_tag;

static SCM mark_window(SCM window_smob)
{
    return SCM_BOOL_F;
}

static size_t free_window(SCM window_smob)
{
    return 0;
}

static int print_window(SCM window_smob, SCM port, scm_print_state *pstate)
{
    client_t *client = (client_t *)SCM_SMOB_DATA(window_smob);

    scm_puts("#<window ", port);
    scm_display(scm_from_unsigned_integer(client->window), port);
    scm_puts(">", port);

    /* success */
    return 1;
}

static void init_window_type(void)
{
    window_tag = scm_make_smob_type("window", sizeof(client_t));
    scm_set_smob_mark(window_tag, mark_window);
    scm_set_smob_free(window_tag, free_window);
    scm_set_smob_print(window_tag, print_window);
}


static SCM nwm_stop(void)
{
    wm_conf.stop = true;
    return SCM_BOOL_T;
}

static SCM count_windows(void)
{
    guint len = g_list_length(client_list);
    return scm_from_unsigned_integer(len);
}

static SCM all_windows(void)
{
    SCM windows = SCM_EOL;
    SCM smob;
    GList *node = client_list;
    while (node) {
        SCM_NEWSMOB(smob, window_tag, (client_t *)node->data);
        windows = scm_append(scm_list_2(windows, scm_list_1(smob)));
        node = node->next;
    }
    return windows;
}

static SCM first_window(void)
{
    SCM window;
    SCM_NEWSMOB(window, window_tag, (client_t *)client_list->data);
    return window;
}

static SCM test_undefined(void)
{
    return SCM_EOL;
}

void *init_scheme(void *data)
{
    scm_c_define_gsubr("nwm-stop", 0, 0, 0, &nwm_stop);
    scm_c_define_gsubr("count-windows", 0, 0, 0, &count_windows);

    scm_c_define_gsubr("all-windows", 0, 0, 0, &all_windows);
    scm_c_define_gsubr("first-window", 0, 0, 0, &first_window);
    scm_c_define_gsubr("test-undefined", 0, 0, 0, &test_undefined);

    init_window_type();

    init_io_buffer_ports();

    return NULL;
}
