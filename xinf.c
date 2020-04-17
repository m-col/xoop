// gcc xinf.c -lX11

#include <stdlib.h>
#include <X11/Xlib.h>

int main()
{
   Display *dpy;

   if (NULL == (dpy = XOpenDisplay(NULL))) {
      exit(1);
   }

   XWarpPointer(dpy, None, DefaultRootWindow(dpy), 0, 0, 0, 0, 0, 0);

   if (XCloseDisplay(dpy)) {
      exit(1);
   }

   return 0;
}
