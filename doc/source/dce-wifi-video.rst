.. include:: replace.txt

Example: video streaming over Wi-Fi with ffmpeg
===============================================

The ``dce-wifi-video`` example runs a real video streaming server and a real
receiver inside |ns3|: both are the unmodified ``ffmpeg`` command line tool,
executed by DCE on the real Linux kernel stack (or on the |ns3| stack), and
the packets go through a simulated 802.11n infrastructure network.

::

   node 0 (server STA)         node 1 (AP)          node 2 (client STA)
   +-------------------+   +--------------+   +------------------------+
   | ffmpeg -re ... udp|   |              |   | ffmpeg -i udp://...    |
   +-------------------+   |              |   | -c copy -f mpegts      |
   | Linux 4.4 (DCE)   |   | Linux 4.4    |   |   /stream.ts           |
   +-------------------+   +--------------+   +------------------------+
   |  802.11n  10.1.1.1|   | 10.1.1.3     |   | 10.1.1.2  802.11n      |
   +-------------------+   +--------------+   +------------------------+
        )))))))))))))))))))))     (((((((((((((((((((((((((

The server reads an MPEG-TS file at its native frame rate (``ffmpeg -re``)
and sends it as MPEG-TS over UDP to the client station. The access point
relays the frames between the two stations, like a real AP does. The client
receives the stream with another ``ffmpeg`` and writes it unchanged
(``-c copy``) to ``/stream.ts`` in its DCE file system, that is
``files-2/stream.ts`` on the host.

Building ffmpeg for DCE
-----------------------

``utils/build_kernel_deps.sh`` builds a minimal, single-threaded, position
independent ``ffmpeg`` 5.1 (the last version whose command line tool works
without threads) with only the file/UDP/RTP protocols, the MPEG-TS demuxer
and muxer and the parsers and decoders needed to probe the copied streams.
It is installed as ``bin_dce/ffmpeg`` with the other DCE binaries. The clip
streamed by the example, ``example/dce-wifi-video-sample.ts`` (47 s, H.264
480x270 and AAC audio), is copied to ``bin_dce/video.ts`` by the DCE build.

Running
-------

The example is built as ``build/bin/dce-wifi-video``. Its main options are:

``--video``
  The MPEG-TS file to stream. A relative path is looked up in the current
  directory and then in the directories of ``DCE_PATH``, so the default
  ``video.ts`` finds the sample clip. Any file that ffmpeg can demux and
  that MPEG-TS can carry (H.264, MPEG-2, AAC, MP2...) can be used.
``--stack``
  ``linux`` (default) runs the real Linux kernel stack on the three nodes,
  ``ns3`` uses the |ns3| TCP/IP stack.
``--realtime``
  Run |ns3| in real-time mode (default), so that the ``-re`` pacing of the
  server matches the wall clock and the video plays at normal speed.
``--fifo``
  Write the received stream to a named pipe (default) so that a player on
  the host can watch it live, instead of a regular file.
``--player``
  A host command started on ``files-2/stream.ts`` while the simulation runs.
  ``{}`` is replaced by that path, otherwise the path is appended.
``--distance``
  Distance between the AP and the client station, in meters (default 10).
``--ping``
  Also run a real ``ping`` from the client to the server (default true).

To watch the video live while it crosses the simulated network::

   $ cd build && ./bin/dce-wifi-video --player="vlc --play-and-exit - < {}"

or start the simulation alone and open ``files-2/stream.ts`` with any
player that can read a pipe. Reading the pipe as standard input (``- < {}``)
avoids the seeks that players attempt on a path.

The standard output of the example reports the frames received by the
client station every second. The output of each DCE process is in
``files-{0,2}/var/log/<pid>/{stdout,stderr}``, and the 802.11 frames seen by
the AP are captured in ``dce-wifi-video-*.pcap``.

Regression test
---------------

With ``--realtime=0 --fifo=0`` the example runs as fast as possible, writes
the received stream to a regular file and, when the simulation ends,
extracts the elementary streams (the concatenated PES payloads) of the sent
and received files and compares them. A stream copy re-multiplexes the
transport layer (new PAT/PMT, PCR, continuity counters, PES headers) but the
elementary streams must be bit-exact. The example exits with an error if
they are not, and ``test.py`` runs it in this mode with both IP stacks when
``bin_dce/ffmpeg`` and ``bin_dce/video.ts`` are present::

   $ ./build/bin/dce-wifi-video --realtime=0 --fifo=0 --stopTime=60
   ...
   Input  .../bin_dce/video.ts: 1187784 bytes, 2 elementary streams
   Output files-2/stream.ts: 1187784 bytes, 2 elementary streams
     input  PID 0x100: 546787 bytes
     input  PID 0x101: 298547 bytes
     output PID 0x100: 546787 bytes
     output PID 0x101: 298547 bytes
   PASS: the received elementary streams are identical to the sent ones
