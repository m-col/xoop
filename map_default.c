#include "common.h"

xcb_xfixes_barrier_t    barriers[4];

extern int debug;
extern int axis;
extern int16_t width, height;
extern const int BOTH_AXES;
extern const int X_ONLY;
extern const int Y_ONLY;

void create_barriers()
{
    uint16_t x1 = 0;
    uint16_t y1 = 0;
    uint16_t x2 = width;
    uint16_t y2 = height;

    if (debug)
        printf("Creating barriers: %i %i %i %i\n", x1, y1, x2, y2);

    if (axis == X_ONLY || axis == BOTH_AXES) {
        create_barrier(&barriers[0], x1, y1, x1, y2);
        create_barrier(&barriers[1], x2, y1, x2, y2);
    };

    if (axis == Y_ONLY || axis == BOTH_AXES) {
        create_barrier(&barriers[2], x1, y1, x2, y1);
        create_barrier(&barriers[3], x1, y2, x2, y2);
    };
}


int map(int32_t ev_x, int32_t ev_y, int16_t *x, int16_t *y) {
    int32_t far_x = width - 1;
    int32_t far_y = height - 1;

    if (ev_x == 0) {
        *x = far_x;
        *y = ev_y;
        return 1;
    } else if (ev_y == 0){
        *x = ev_x;
        *y = far_y;
        return 1;
    } else if (ev_x == far_x){
        *y = ev_y;
        return 1;
    } else if (ev_y == far_y){
        *x = ev_x;
        return 1;
    };
    return 0;
}
