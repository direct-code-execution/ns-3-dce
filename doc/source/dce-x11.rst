.. include:: replace.txt

Graphical applications: X11 clients inside DCE
==============================================

DCE applications can open windows on the host X display. The applications
run entirely inside the simulation (their X11 client libraries are loaded
by DCE against its libc, their sockets are the ones of the simulated node)
and only their connection to the X server is handed to the host.

How it works
------------

The ``DceHostUnixSocketPaths`` global value lists AF_UNIX socket path
prefixes. When a DCE process connects a stream socket to a matching path,
DCE replaces the never-connected local socket by a ``HostSocketFd``
(``model/host-socket-fd.h``): a connected socket of the host seen as a
regular file descriptor of the DCE process. The host socket is kept
non-blocking, blocking reads and writes of the application are emulated by
sleeping the DCE task, and the readiness of the socket is re-checked every
millisecond while a DCE task polls it. Since the host side lives in
wall-clock time, this requires ``ns3::RealtimeSimulatorImpl``.

``DceX11Helper`` (``helper/dce-x11-helper.h``) does the three things a
simulation needs: ``Enable()`` adds ``/tmp/.X11-unix/`` to the global
value, ``InstallAuthority(node)`` writes the host's X authority cookies
into the node's file system (``files-<node>/.Xauthority``) as wildcard
entries, because a DCE application sees its node's hostname and
host-bound cookies would not match, and ``SetEnvironment(dce)`` sets
``DISPLAY``, ``XAUTHORITY`` and ``HOME`` for the applications the helper
installs.

Applications are built like any DCE application (``-fPIC``, ``-pie
-rdynamic``, no fortified libc calls or stack protector); the X11 client
libraries of the host (libX11, libxcb, libXext, libXv, libXft, FLTK...)
are loaded as they are.

Examples
--------

``dce-x11-hello``
  Runs ``x11-hello`` (``example/x11-hello.cc``, a minimal Xlib client) on a
  node and fails if it cannot draw its window. ``test.py`` runs it when
  ``DISPLAY`` is set; CI runs the tests under ``xvfb-run``.

``dce-wifi-video --viewer=1``
  The receiving ffmpeg of the video streaming example decodes the stream
  and displays it in a window with its ``xv`` output device, from inside the
  simulated client station (see :doc:`dce-wifi-video`). XVideo is not
  available on Xvfb, so this one needs a real display.

``dce-dillo``
  A real graphical web browser. Dillo (HTML, CSS, images, no JavaScript)
  runs on a Wi-Fi station, fetches pages from a real thttpd running on
  another station through the access point, and draws its window on the
  host display. The example writes a small web site in ``files-0/`` and
  gives the browser the host's ``/usr`` and ``/etc`` (symlinks in
  ``files-2/``) so that fontconfig finds its configuration and the fonts.
  Closing the browser window ends the simulation::

     $ ./bin/dce-dillo
     $ ./bin/dce-dillo --url=http://10.1.1.1/big.html --distance=40

  ``utils/build_kernel_deps.sh`` builds Dillo 3.1.1 for DCE (single
  process, no threaded DNS, no TLS) when the FLTK headers are installed.

Limits
------

This path only works for single-process clients whose threads, if any, go
through pthreads (cooperative fibers in DCE). Multi-process browsers
(Firefox, Chromium, Servo, Ladybird) and VLC are out of reach. There is no
sound: it would need a PulseAudio/PipeWire client running under DCE.

Event loops: eventfd and epoll
------------------------------

DCE provides ``eventfd(2)``, ``epoll(7)`` (``epoll_create``,
``epoll_create1``, ``epoll_ctl``, ``epoll_wait``, ``epoll_pwait``) and
``pipe2(2)`` for the applications that use them (GLib, Qt, libevent,
libuv...). epoll is level-triggered; ``EPOLLET`` is accepted and treated as
level-triggered, ``EPOLLONESHOT`` is honoured, and waiting on an epoll
descriptor uses the same mechanism as ``poll(2)``. ``test-eventfd`` and
``test-epoll`` in the DCE test suite cover them.
