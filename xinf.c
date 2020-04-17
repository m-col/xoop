#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

int main()
{
    Display *dpy;
    Window root;
    Window win;
    XSetWindowAttributes attrs;
    XEvent ev; 

    if (NULL == (dpy = XOpenDisplay(NULL))) {
	exit(1);
    }

    root = RootWindow(dpy, DefaultScreen(dpy));
    attrs.event_mask = KeyPressMask;
    Atom window_type = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
    long value = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_NOTIFICATION", False);

    win = XCreateWindow(
        dpy, root, 10,10,200,200, 0, CopyFromParent, InputOnly, 0, 0, &attrs
    );
    XChangeProperty(dpy, win, window_type, 4, 32,
                    PropModeReplace, (unsigned char *)&value, 1);

    XSelectInput(dpy, win, EnterWindowMask);
    XMapWindow(dpy, win);
    XFlush(dpy);

    while(1) {
        XNextEvent(dpy, &ev);
	XWarpPointer(dpy, None, root, 0, 0, 0, 0, 0, 0);
    }   

    if (XCloseDisplay(dpy)) {
	exit(1);
    }

    return 0;
}
