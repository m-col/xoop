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
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include <xcb/xfixes.h>
#include <xcb/xinput.h>

#define PROGNAME "xoop"

xcb_connection_t	*conn;
xcb_screen_t		*screen;
xcb_input_device_id_t	deviceid;
xcb_xfixes_barrier_t    barriers[4];

enum axis_t { NO_AXIS = 0, X_AXIS = 1 << 0, Y_AXIS = 1 << 1, BOTH_AXES = X_AXIS | Y_AXIS };
enum axis_t axis = NO_AXIS;

int debug = 0;
uint8_t op_randr = 0;
uint8_t op_xfixes = 0;
uint8_t op_xinput = 0;
int16_t width, height;


void exit_angrily(char msg[])
{
    fprintf(stderr, "%s", msg);
    xcb_disconnect(conn);
    exit(EXIT_FAILURE);
}


void create_barrier(xcb_xfixes_barrier_t *barrier, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    xcb_generic_error_t *error;
    *barrier = xcb_generate_id(conn);
    xcb_void_cookie_t cookie = xcb_xfixes_create_pointer_barrier_checked(
	conn, *barrier, screen->root, x1, y1, x2, y2, 0, 0, NULL
    );
    if ((error = xcb_request_check(conn, cookie))) {
	free(error);
	exit_angrily("Could not create barriers.\n");
    }
    free(error);
}


void create_barriers()
{
    uint16_t x1 = 0;
    uint16_t y1 = 0;
    uint16_t x2 = width;
    uint16_t y2 = height;

    if (debug)
	printf("Creating barriers: %i %i %i %i\n", x1, y1, x2, y2);

    if (axis & X_AXIS) {
	create_barrier(&barriers[0], x1, y1, x1, y2);
	create_barrier(&barriers[1], x2, y1, x2, y2);
    };

    if (axis & Y_AXIS) {
	create_barrier(&barriers[2], x1, y1, x2, y1);
	create_barrier(&barriers[3], x1, y2, x2, y2);
    };
}


void delete_barriers()
{
    for (int i = 0; i < 4; i++) {
	if (debug)
	    printf("Deleting barrier: %i/4\n", i + 1);

	xcb_xfixes_delete_pointer_barrier(conn, barriers[i]);
    }
}


void exit_nicely(int sig)
{
    (void)(sig); // Unused
    delete_barriers();
    xcb_disconnect(conn);
    exit(EXIT_SUCCESS);
}


void check_xfixes()
{
    xcb_xfixes_query_version(conn, 5, 0);
    const xcb_query_extension_reply_t *ext = xcb_get_extension_data(conn, &xcb_xfixes_id);
    if (!ext || !ext->present) {
        printf("XFixes extension not available.\n");
	exit(EXIT_FAILURE);
    }
    op_xfixes = ext->first_event;
}


void check_randr()
{
    const xcb_query_extension_reply_t *ext = xcb_get_extension_data(conn, &xcb_randr_id);
    if (!ext || !ext->present)
        exit_angrily("Randr extension not available.\n");
    op_randr = ext->first_event;
    xcb_randr_select_input(conn, screen->root, XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE);
}


void check_xinput()
{
    const xcb_query_extension_reply_t *ext = xcb_get_extension_data(conn, &xcb_input_id);
    if (!ext || !ext->present)
        exit_angrily("XInput extension not available.\n");
    struct {
        xcb_input_event_mask_t head;
        xcb_input_xi_event_mask_t mask;
    } mask;
    mask.head.deviceid = XCB_INPUT_DEVICE_ALL_MASTER;
    mask.head.mask_len = sizeof(mask.mask) / sizeof(uint32_t);
    mask.mask = XCB_INPUT_XI_EVENT_MASK_BARRIER_HIT;
    xcb_input_xi_select_events(conn, screen->root, 1, &mask.head);
}


void loop_cursor(xcb_generic_event_t *generic_event)
{
    xcb_input_barrier_hit_event_t *event = (xcb_input_barrier_hit_event_t *)generic_event;
    int16_t x = 0;
    int16_t y = 0;
    int32_t far_x = width - 1;
    int32_t far_y = height - 1;
    int32_t ev_x = (int32_t)(event->root_x / (double)UINT16_MAX);
    int32_t ev_y = (int32_t)(event->root_y / (double)UINT16_MAX);

    if (axis & X_AXIS && ev_x == 0) {
      x = far_x;
      y = ev_y;
    } else if (axis & Y_AXIS && ev_y == 0){
      x = ev_x;
      y = far_y;
    } else if (axis & X_AXIS && ev_x == far_x){
      y = ev_y;
    } else if (axis & Y_AXIS && ev_y == far_y){
      x = ev_x;
    };
    xcb_warp_pointer(conn, XCB_NONE, screen->root, 0, 0, 0, 0, x, y);

    if (debug)
	printf("Warp: (%i, %i) to (%i, %i)\n", ev_x, ev_y, x, y);
}


void reset_screen(xcb_generic_event_t *generic_event)
{
    xcb_randr_screen_change_notify_event_t *event = (xcb_randr_screen_change_notify_event_t *)generic_event;
    width = event->width;
    height = event->height;
    delete_barriers();
    create_barriers();
    if (debug)
	printf("Configured screens.\n");
}


void event_loop()
{
    xcb_generic_event_t *event;
    xcb_flush(conn);

    while((event = xcb_wait_for_event(conn))) {
	if (event->response_type == XCB_GE_GENERIC) {
	    loop_cursor(event); /* we only receive 1 event from xinput */
	} else if (event->response_type == op_randr + XCB_RANDR_SCREEN_CHANGE_NOTIFY) {
	    reset_screen(event);
	} else {
	    printf("Unknown event: %d\n", event->response_type);
	};
	xcb_flush(conn);
	free(event);
    }
}


void print_help()
{
    printf(
	PROGNAME " [-h|-f|-d]\n"
	"\n"
	"    -h    print help\n"
	"    -x    xoop only the x axis\n"
	"    -y    xoop only the y axis\n"
	"    -f    fork\n"
	"    -d    print debug information\n"
    );
}


int main(int argc, char *argv[])
{
    int opt;
    int to_fork = 0;
    int pid;

    while ((opt = getopt(argc, argv, "hfdxy")) != -1) {
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
	    case 'x':
    axis |= X_AXIS;
		break;
	    case 'y':
    axis |= Y_AXIS;
		break;
        }
    }

    if (axis == NO_AXIS)
	axis = BOTH_AXES;

    conn = xcb_connect(NULL, NULL);
    if (xcb_connection_has_error(conn))
	exit(EXIT_FAILURE);

    screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
    width = screen->width_in_pixels;
    height = screen->height_in_pixels;
    check_xfixes();
    check_randr();
    check_xinput();
    create_barriers();

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

    exit_nicely(0);
    return 0;
}
