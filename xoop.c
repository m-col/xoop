/*
Copyright (c) 2020, Matt Colligan All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <xcb/randr.h>
#include <xcb/shape.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>

#define PROGNAME "xoop"

xcb_connection_t    *conn;
xcb_screen_t	    *screen;
xcb_window_t	     wid;

int debug = 0;
int randr_base = 0;


void set_window_type() {
    xcb_intern_atom_reply_t *wm_window_type,
			    *wm_window_type_dock,
			    *wm_state,
			    *wm_state_above,
			    *wm_desktop;

    wm_window_type = xcb_intern_atom_reply(
	conn,
	xcb_intern_atom(conn, 0, 19, "_NET_WM_WINDOW_TYPE"),
	NULL
    );
    wm_window_type_dock = xcb_intern_atom_reply(
	conn,
	xcb_intern_atom(conn, 0, 24, "_NET_WM_WINDOW_TYPE_DOCK"),
	NULL
    );
    xcb_change_property(
	conn, XCB_PROP_MODE_REPLACE, wid, wm_window_type->atom, XCB_ATOM_ATOM, 32, 1, &wm_window_type_dock->atom
    );

    wm_state = xcb_intern_atom_reply(
	conn,
	xcb_intern_atom(conn, 0, 13, "_NET_WM_STATE"),
	NULL
    );
    wm_state_above = xcb_intern_atom_reply(
	conn,
	xcb_intern_atom(conn, 0, 19, "_NET_WM_STATE_ABOVE"),
	NULL
    );
    xcb_change_property(
	conn, XCB_PROP_MODE_REPLACE, wid, wm_state->atom, XCB_ATOM_ATOM, 32, 1, &wm_state_above->atom
    );

    wm_desktop = xcb_intern_atom_reply(
        conn,
        xcb_intern_atom(conn, 0, 15, "_NET_WM_DESKTOP"),
        NULL
    );
    xcb_change_property(
        conn, XCB_PROP_MODE_REPLACE, wid, wm_desktop->atom, XCB_ATOM_INTEGER, 1, 1, (uint32_t []){0xFFFFFFFF}
    );

    xcb_configure_window(conn, wid, XCB_CONFIG_WINDOW_STACK_MODE, (uint32_t []){XCB_STACK_MODE_ABOVE});

    xcb_change_property(
	conn, XCB_PROP_MODE_REPLACE, wid, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, 4, PROGNAME
    );

    free(wm_window_type);
    free(wm_window_type_dock);
    free(wm_desktop);
}


void set_window_shape(uint16_t width, uint16_t height)
{
    xcb_pixmap_t pixmap = xcb_generate_id(conn);
    xcb_gcontext_t gc = xcb_generate_id(conn);
    xcb_create_pixmap(conn, 1, pixmap, wid, width, height);
    xcb_create_gc(
	conn, gc, pixmap, XCB_GC_FOREGROUND, &screen->white_pixel
    );
    xcb_rectangle_t rect = {0, 0, width, height};
    xcb_poly_fill_rectangle(conn, pixmap, gc, 1, &rect);
    xcb_shape_mask(
	conn, XCB_SHAPE_SO_SET, XCB_SHAPE_SK_INPUT, wid, 0, 0, pixmap
    );

    xcb_pixmap_t pixmap2 = xcb_generate_id(conn);
    xcb_gcontext_t gc2 = xcb_generate_id(conn);
    xcb_create_pixmap(conn, 1, pixmap2, wid, width, height);
    xcb_create_gc(
	conn, gc2, pixmap2, XCB_GC_FOREGROUND, &screen->white_pixel
    );
    xcb_rectangle_t rect2 = {1, 1, width - 2, height - 2};
    xcb_poly_fill_rectangle(conn, pixmap2, gc2, 1, &rect2);
    xcb_shape_mask(
	conn, XCB_SHAPE_SO_SUBTRACT, XCB_SHAPE_SK_INPUT, wid, 0, 0, pixmap2
    );

    if (debug) {
	xcb_shape_mask(
	    conn, XCB_SHAPE_SO_SET, XCB_SHAPE_SK_BOUNDING, wid, 0, 0, pixmap
	);
	xcb_shape_mask(
	    conn, XCB_SHAPE_SO_SUBTRACT, XCB_SHAPE_SK_BOUNDING, wid, 0, 0, pixmap2
	);
    };

    xcb_free_pixmap(conn, pixmap);
    xcb_free_pixmap(conn, pixmap2);
    xcb_free_gc(conn, gc);
    xcb_free_gc(conn, gc2);
}


void setup_window()
{
    int class = XCB_WINDOW_CLASS_INPUT_ONLY;
    uint32_t value_mask = XCB_CW_EVENT_MASK;
    uint32_t value_list[1] = {XCB_EVENT_MASK_ENTER_WINDOW};
    uint32_t value_list_debug[2] = {screen->white_pixel, XCB_EVENT_MASK_ENTER_WINDOW};
    uint32_t *values = value_list;

    if (debug) {
	class = XCB_WINDOW_CLASS_INPUT_OUTPUT;
	value_mask |= XCB_CW_BACK_PIXEL;
	values = value_list_debug;
    };

    wid = xcb_generate_id(conn);
    xcb_create_window(
	conn,
	XCB_COPY_FROM_PARENT,
	wid,
	screen->root,
	0,
	0,
	screen->width_in_pixels,
	screen->height_in_pixels,
	0,
	class,
	XCB_COPY_FROM_PARENT,
	value_mask,
	values
    );

    set_window_type();
    set_window_shape(screen->width_in_pixels, screen->height_in_pixels);
    xcb_map_window(conn, wid);
}


void configure_randr()
{
    const xcb_query_extension_reply_t *randr = xcb_get_extension_data(conn, &xcb_randr_id);
    if (randr->present) {
	randr_base = randr->first_event;
	xcb_randr_select_input(conn, screen->root, XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE);
    };
}


void event_loop()
{
    xcb_generic_event_t *event;
    xcb_enter_notify_event_t *entry;
    xcb_randr_screen_change_notify_event_t *change;
    int16_t x = 0;
    int16_t y = 0;

    int16_t far_x = screen->width_in_pixels - 1;
    int16_t far_y = screen->height_in_pixels - 1;

    xcb_flush(conn);

    while((event = xcb_wait_for_event(conn))) {
	switch (event->response_type) {

	    case XCB_ENTER_NOTIFY:
		entry = (xcb_enter_notify_event_t *)event;
		if (entry->event_x == 0) {
		    x = far_x;
		    y = entry->event_y;
		} else if (entry->event_y == 0){
		    x = entry->event_x;
		    y = far_y;
		} else if (entry->event_x == far_x){
		    x = 0;
		    y = entry->event_y;
		} else if (entry->event_y == far_y){
		    x = entry->event_x;
		    y = 0;
		};
		xcb_warp_pointer(conn, XCB_NONE, screen->root, 0, 0, 0, 0, x, y);
		if (debug) {
		    printf("Entry: %d, %d\n", entry->event_x, entry->event_y);
		    printf("Warp: (%d, %d) to (%d, %d)\n", entry->event_x, entry->event_y, x, y);
		};
		break;

	    default:
		if (event->response_type == randr_base + XCB_RANDR_SCREEN_CHANGE_NOTIFY) {
		    change = (xcb_randr_screen_change_notify_event_t *)event;
		    set_window_shape(change->width, change->height);
		    far_x = change->width - 1;
		    far_y = change->height - 1;
		    if (debug) printf("Screen changed.\n");
		} else {
		    printf("Unknown event: %d\n", event->response_type);
		};

	}
	xcb_flush(conn);
	free(event);
    }
    free(entry);
}


void exit_nicely()
{
    xcb_unmap_window(conn, wid);
    xcb_disconnect(conn);
    exit(EXIT_SUCCESS);
}


void print_help()
{
    printf(
	PROGNAME " [-h|-f|-d]\n"
	"\n"
	"    -h    print help\n"
	"    -f    fork\n"
	"    -d    print debug information\n"
    );
}


int main(int argc, char *argv[])
{
    int opt;
    int to_fork = 0;
    int pid;

    while ((opt = getopt(argc, argv, "hfd")) != -1) {
        switch (opt) {
	    case 'h':
		print_help();
		exit(EXIT_SUCCESS);
	    case 'f':
		to_fork = 1;
		break;
	    case 'd':
		debug = 1;
		break;
        }
    }

    conn = xcb_connect(NULL, NULL);
    if (xcb_connection_has_error(conn))
	exit(EXIT_FAILURE);

    screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
    configure_randr();
    setup_window();

    signal(SIGINT, exit_nicely);
    signal(SIGTERM, exit_nicely);
    signal(SIGHUP, exit_nicely);

    if (to_fork) {
	pid = fork();
	if (pid > 0) {
	    exit(EXIT_SUCCESS);
	} else if (pid < 0) {
	    exit(EXIT_FAILURE);
	}
    }

    event_loop();

    exit_nicely(EXIT_SUCCESS);
    return 0;
}
