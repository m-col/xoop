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


#include <stdio.h>
#include <stdlib.h>
#include <xcb/shape.h>
#include <xcb/xcb.h>
#include <unistd.h>

#define PROGNAME "xoop"

xcb_connection_t    *conn;
xcb_screen_t	    *screen;


void set_window_type(xcb_window_t wid) {
    xcb_intern_atom_reply_t *reply1, *reply2, *reply3;

    reply1 = xcb_intern_atom_reply(
	conn,
	xcb_intern_atom(conn, 0, 19, "_NET_WM_WINDOW_TYPE"),
	NULL
    );
    reply2 = xcb_intern_atom_reply(
	conn,
	xcb_intern_atom(conn, 0, 24, "_NET_WM_WINDOW_TYPE_DOCK"),
	NULL
    );
    xcb_change_property(
	conn, XCB_PROP_MODE_REPLACE, wid, reply1->atom, XCB_ATOM_ATOM, 32, 1, &reply2->atom
    );

    reply3 = xcb_intern_atom_reply(
        conn,
        xcb_intern_atom(conn, 0, 15, "_NET_WM_DESKTOP"),
        NULL
    );
    xcb_change_property(
        conn, XCB_PROP_MODE_REPLACE, wid, reply3->atom, XCB_ATOM_INTEGER, 1, 1, (uint32_t []){ 0xFFFFFFFF }
    );

    free(reply1);
    free(reply2);
    free(reply3);
}


void set_window_shape(xcb_window_t wid)
{
    xcb_gcontext_t gc = xcb_generate_id(conn);
    xcb_pixmap_t pixmap = xcb_generate_id(conn);

    xcb_create_pixmap(
	conn, 1, pixmap, wid, screen->width_in_pixels, screen->height_in_pixels
    );

    const uint32_t values[] = {screen->white_pixel};

    xcb_create_gc(
	conn, gc, pixmap, XCB_GC_FOREGROUND, values
    );

    xcb_rectangle_t rect = {
	1, 1, screen->width_in_pixels - 2, screen->height_in_pixels - 2
    };
    xcb_poly_fill_rectangle(
	conn, pixmap, gc, 1, &rect
    );

    xcb_shape_mask(
	conn, XCB_SHAPE_SO_SUBTRACT, XCB_SHAPE_SK_INPUT, wid, 0, 0, pixmap
    );

    xcb_free_gc(conn, gc);
    xcb_free_pixmap(conn, pixmap);
}


xcb_window_t setup_window()
{
    xcb_window_t wid = xcb_generate_id(conn);

    uint32_t values = 0;
    values |= XCB_EVENT_MASK_ENTER_WINDOW;

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
	XCB_WINDOW_CLASS_INPUT_ONLY,
	XCB_COPY_FROM_PARENT,
	XCB_CW_EVENT_MASK,
	&values
    );

    set_window_type(wid);
    set_window_shape(wid);

    uint32_t above = XCB_STACK_MODE_ABOVE;
    xcb_configure_window(conn, wid, XCB_CONFIG_WINDOW_STACK_MODE, &above);

    xcb_change_property(
	conn, XCB_PROP_MODE_REPLACE, wid, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, 4, PROGNAME
    );

    xcb_map_window(conn, wid);
    xcb_flush(conn);
    return wid;
}


void event_loop()
{
    xcb_generic_event_t *event;
    xcb_enter_notify_event_t *entry;
    int16_t x = 0;
    int16_t y = 0;

    int16_t far_x = screen->width_in_pixels - 1;
    int16_t far_y = screen->height_in_pixels - 1;

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
	    xcb_warp_pointer(
		conn, XCB_NONE, screen->root, 0, 0, 0, 0, x, y
	    );
	    xcb_flush(conn);
#ifdef DEBUG
	    printf("(%d, %d) to (%d, %d)\n", entry->event_x, entry->event_y, x, y);
#endif
	    break;
	}

    }
    free(event);
    free(entry);
}


void print_help()
{
    printf(
	PROGNAME " [-h|-f]\n"
	"\n"
	"    -h    print help\n"
	"    -f    fork\n"
    );
}


int main(int argc, char *argv[])
{
    int opt;
    int to_fork = 0;
    int pid;
    xcb_window_t wid;

    while ((opt = getopt(argc, argv, "hf")) != -1) {
        switch (opt) {
	    case 'h':
		print_help();
		exit(EXIT_SUCCESS);
	    case 'f':
		to_fork = 1;
		break;
        }
    }

    conn = xcb_connect(NULL, NULL);
    if (xcb_connection_has_error(conn))
	exit(EXIT_FAILURE);

    screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
    wid = setup_window();

    if (to_fork) {
	pid = fork();
	if (pid > 0) {
	    exit(EXIT_SUCCESS);
	} else if (pid < 0) {
	    exit(EXIT_FAILURE);
	}
    }

    event_loop();

    xcb_unmap_window(conn, wid);
    xcb_disconnect(conn);

    return 0;
}
