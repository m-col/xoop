#include <stdio.h>
#include <stdlib.h>
#include <xcb/xfixes.h>

void create_barriers();

void create_barrier(xcb_xfixes_barrier_t *barrier, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

int map(int32_t ev_x, int32_t ev_y, int16_t *x, int16_t *y);

