/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef DCE_X11_HELPER_H
#define DCE_X11_HELPER_H

#include "dce-application-helper.h"
#include "ns3/node.h"
#include <string>

namespace ns3 {

/**
 * \brief Let DCE applications open windows on the host X display.
 *
 * X11 clients built for DCE run entirely inside the simulation; only their
 * connection to the X server is handed to the host, through the
 * DceHostUnixSocketPaths global value and HostSocketFd. Since the X server
 * lives in wall-clock time, use ns3::RealtimeSimulatorImpl.
 *
 * Typical use:
 * \code
 *   DceX11Helper::Enable ();                  // before Simulator::Run
 *   DceX11Helper::InstallAuthority (node);    // files-<node>/.Xauthority
 *   DceApplicationHelper dce;
 *   dce.SetBinary ("x11-hello");
 *   DceX11Helper::SetEnvironment (dce);       // DISPLAY, XAUTHORITY, HOME
 * \endcode
 */
class DceX11Helper
{
public:
  /**
   * Redirect AF_UNIX connections of DCE applications to /tmp/.X11-unix/ (the
   * X server sockets) to the host, by adding the prefix to the
   * DceHostUnixSocketPaths global value.
   */
  static void Enable (void);

  /**
   * Write the X authority cookies of the host into the file system of the
   * node (files-<node>/.Xauthority), as wildcard entries: the hostname seen
   * by a DCE application is the one of its node, not the host's, so entries
   * bound to the host name would never match.
   * \param node the node whose applications will connect to the display
   * \param authority host authority file, default $XAUTHORITY or ~/.Xauthority
   * \return true if a file was written
   */
  static bool InstallAuthority (Ptr<Node> node, std::string authority = "");

  /**
   * Add DISPLAY (the host's, or the given one), XAUTHORITY=/.Xauthority and
   * HOME=/ to the environment of the applications the helper will install.
   */
  static void SetEnvironment (DceApplicationHelper &dce, std::string display = "");

  /** \return the display the clients connect to: $DISPLAY of the host. */
  static std::string GetDisplay (void);

  /**
   * Let the applications the helper installs use the host's D-Bus session
   * bus: the AF_UNIX path of $DBUS_SESSION_BUS_ADDRESS is added to
   * DceHostUnixSocketPaths and the variable is set in the applications'
   * environment. GApplication based programs (GTK) need a bus to register
   * on, and GIO would otherwise try to spawn dbus-launch.
   * \return false if the host has no unix:path= session bus address
   */
  static bool UseSessionBus (DceApplicationHelper &dce);

  /**
   * Let the applications the helper installs play sound on the host's
   * PulseAudio (or PipeWire) server: its native socket
   * ($XDG_RUNTIME_DIR/pulse/native, or $PULSE_SERVER) is added to
   * DceHostUnixSocketPaths, the authentication cookie is copied into the
   * node's file system (files-<node>/.config/pulse/cookie, HOME is /), and
   * a client configuration without shared memory transport is written
   * (the server and the application do not share memory).
   * \param node the node whose applications will play sound
   * \return false if the host has no PulseAudio socket
   */
  static bool UsePulseAudio (Ptr<Node> node, DceApplicationHelper &dce);
};

} // namespace ns3

#endif /* DCE_X11_HELPER_H */
