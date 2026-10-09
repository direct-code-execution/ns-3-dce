you .. include:: replace.txt

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
-rdynamic``, no fortified libc calls or stack protector, and
``-ftls-model=global-dynamic`` if the program itself has thread local
variables: DCE loads a program like a shared object, so the fixed ``%fs``
offsets of the local-exec model an executable normally uses would point
into the simulator's own thread local storage); the X11 client libraries
of the host (libX11, libxcb, libXext, libXv, libXft, FLTK...) are loaded
as they are.

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

``dce-browser``
  A real graphical web browser. Dillo (HTML, CSS, images, no JavaScript)
  runs on a Wi-Fi station, fetches pages from a real thttpd running on
  another station through the access point, and draws its window on the
  host display. The example writes a small web site in ``files-0/`` and
  gives the browser the host's ``/usr`` and ``/etc`` (symlinks in
  ``files-2/``) so that fontconfig finds its configuration and the fonts.
  Closing the browser window ends the simulation::

     $ ./bin/dce-browser
     $ ./bin/dce-browser --url=http://10.1.1.1/big.html --distance=40

  ``utils/build_kernel_deps.sh`` builds Dillo 3.1.1 for DCE (single
  process, no threaded DNS, no TLS) when the FLTK headers are installed.

``dce-browser --browser=northstar``
  Northstar, a GTK4 web browser with its own engine, CSS, JavaScript
  (QuickJS) and software rendering, built for DCE by
  ``utils/build_kernel_deps.sh`` when the GTK4 development packages and
  meson are installed (against a minimal static libcurl, with ``-fPIC`` and
  without LTO: a PIE compiled executable gets copy relocations of
  stdin/stdout/stderr that DCE's per-process stdio cannot reach, and its
  ``main`` exported from the project's version script). The example runs it
  with ``NS_NO_SANDBOX=1 --no-watchdog`` (its Landlock/seccomp sandbox would
  apply to the simulator, and its supervisor process cannot be spawned),
  the GTK4 environment above, and the host's D-Bus session bus through the
  passthrough (``DceX11Helper::UseSessionBus``): GApplication registers on
  the bus, and GIO would otherwise try to spawn dbus-launch.

Sound
-----

The same passthrough carries sound: ``DceX11Helper::UsePulseAudio(node,
dce)`` adds the host's PulseAudio (or PipeWire) native socket to
``DceHostUnixSocketPaths``, copies the authentication cookie into the
node's file system and writes a client configuration without shared memory
transport, and the application plays through libpulse running inside DCE.
``dce-wifi-video --viewer=1`` plays the soundtrack this way with ffmpeg's
``pulse`` output device (``--audio=0`` to stay silent). The passthrough
rewrites the ``SCM_CREDENTIALS`` libpulse sends to the simulator's real
credentials, which is what the server authenticates.

Checking the traffic
--------------------

The examples that generate traffic capture it at the access point
(``<example>-1-0.pcap``) and check it before exiting, so that they fail as
tests when the expected traffic is missing: ``DcePcapCheck``
(``helper/dce-pcap-check.h``) reads an ns-3 pcap back (802.11 frames, with
or without QoS control and A-MPDU aggregation, Ethernet, raw IPv4) and
counts TCP/UDP packets, payload bytes and HTTP request lines.
``dce-wifi-video`` requires the UDP stream to the receiver's port,
``dce-browser`` at least ``--minRequests`` HTTP requests (1 by default; the
test of the JavaScript page asks for 8 in 16 s).

``dce-browser`` also starts ``numbers-server`` (``example/numbers-server.cc``,
a tiny HTTP server built as a DCE application) on the server station: its
page, ``http://10.1.1.1:8080/``, polls ``/next`` from JavaScript twice a
second and shows the Fibonacci numbers the server answers, each request an
HTTP exchange over the simulated link. The site also serves the video
sample of ``dce-wifi-video``, as the MPEG-TS file and, on
``http://10.1.1.1/video.html``, in an HTML5 ``<video>`` element as an MPEG-1
program stream (``bin_dce/video.mpg``, made from the sample by the DCE
ffmpeg at build time), the format Northstar's own player decodes.
Northstar's ``<video>`` element is picture only, so the page also puts the
same file in an ``<audio>`` element: with the SDL2 audio mixer built in, the
MP2 track plays on the host through PulseAudio (SDL2 itself runs inside
the simulation, its mixer thread a DCE fiber). The page starts the sound
when the picture starts and keeps the picture on the sound's clock. The
clip is 320x180 at 24 frames per second because Northstar decodes a whole
clip into memory up front and stops at 256 MB of frames::

   $ ./bin/dce-browser --browser=northstar --url=http://10.1.1.1/video.html

Limits
------

This path only works for single-process clients whose threads, if any, go
through pthreads (cooperative fibers in DCE). Multi-process browsers
(Firefox, Chromium, Servo, Ladybird) and VLC are out of reach.

GTK4 applications
-----------------

GLib, GObject, GIO and GTK 4 run inside DCE: ``example/gtk4-hello.cc`` is
a minimal GTK4 application built as a DCE application when libgtk-4-dev is
installed, and ``test.py`` runs it with ``dce-x11-hello`` under Xvfb. What
this exercises in DCE: eventfd and poll based main loops, GLib's futex
based locks (``syscall(SYS_futex)`` is emulated, see ``model/dce-futex.cc``),
16 byte aligned ``malloc`` and ``posix_memalign``, the file and process
system calls GIO probes (statx, extended attributes, inotify, posix_spawn:
reported as unsupported in the way GIO handles), and the X11 connection.
Run GTK4 applications with ``GSK_RENDERER=cairo`` (no OpenGL in the
simulation), ``GDK_BACKEND=x11``, ``GSETTINGS_BACKEND=memory`` (no dconf),
``NO_AT_BRIDGE=1`` (no accessibility bus), and give them the host's
``/usr`` and ``/etc`` for fonts and toolkit data
(``dce-x11-hello --hostfs=1``)::

   $ ./bin/dce-x11-hello --binary=gtk4-hello --hostfs=1 \
       --env=GSK_RENDERER=cairo,GDK_BACKEND=x11,GSETTINGS_BACKEND=memory,NO_AT_BRIDGE=1

Event loops: eventfd and epoll
------------------------------

DCE provides ``eventfd(2)``, ``epoll(7)`` (``epoll_create``,
``epoll_create1``, ``epoll_ctl``, ``epoll_wait``, ``epoll_pwait``) and
``pipe2(2)`` for the applications that use them (GLib, Qt, libevent,
libuv...). epoll is level-triggered; ``EPOLLET`` is accepted and treated as
level-triggered, ``EPOLLONESHOT`` is honoured, and waiting on an epoll
descriptor uses the same mechanism as ``poll(2)``. ``test-eventfd`` and
``test-epoll`` in the DCE test suite cover them.
