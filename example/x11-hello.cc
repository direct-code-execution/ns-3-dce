/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
//
// Minimal X11 client built as a DCE application: opens the display, shows a
// window with a moving box for <seconds> (default 5) and exits with 0, or
// with 1 if the display cannot be opened. Run by dce-x11-hello to exercise
// the host socket passthrough of DCE (DceHostUnixSocketPaths).
//
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int
main (int argc, char **argv)
{
  int seconds = argc > 1 ? atoi (argv[1]) : 5;
  Display *display = XOpenDisplay (NULL);
  if (display == NULL)
    {
      fprintf (stderr, "x11-hello: cannot open display '%s'\n",
               getenv ("DISPLAY") ? getenv ("DISPLAY") : "(unset)");
      return 1;
    }
  int screen = DefaultScreen (display);
  Window window = XCreateSimpleWindow (display, RootWindow (display, screen), 10, 10, 400, 200, 1,
                                       BlackPixel (display, screen), WhitePixel (display, screen));
  XStoreName (display, window, "x11-hello: drawn from inside ns-3 DCE");
  XSelectInput (display, window, ExposureMask | KeyPressMask);
  XMapWindow (display, window);
  GC gc = DefaultGC (display, screen);
  const char *message = "Hello from a process running inside the ns-3 simulation";
  struct timespec start, now;
  clock_gettime (CLOCK_MONOTONIC, &start);
  int frames = 0;
  while (true)
    {
      while (XPending (display))
        {
          XEvent event;
          XNextEvent (display, &event);
          if (event.type == KeyPress)
            {
              seconds = 0;
            }
        }
      XClearWindow (display, window);
      XDrawString (display, window, gc, 20, 30, message, strlen (message));
      XFillRectangle (display, window, gc, 20 + (frames * 4) % 340, 60, 40, 40);
      XFlush (display);
      frames++;
      usleep (40000);
      clock_gettime (CLOCK_MONOTONIC, &now);
      if (now.tv_sec - start.tv_sec >= seconds)
        {
          break;
        }
    }
  printf ("x11-hello: drew %d frames on display %s\n", frames, DisplayString (display));
  XCloseDisplay (display);
  return 0;
}
