/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Frederic Urbani <frederic.urbani@inria.fr>
 *
 * Do same test that the ccnx.0.4.0 test named test_short_stuff, but without using shell script.
 */


#include "ns3/network-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/dce-module.h"

using namespace ns3;

void
CreateFiles ()
{
  std::ofstream osf ("/tmp/README", std::fstream::trunc);

  osf << "The wanted data is here :)" ;
  osf.close ();

  std::ofstream osf2 ("/tmp/100", std::fstream::trunc);

  for (int i = 1; i <= 100; i++)
    {
      osf2 << i << std::endl;
    }
  osf2.close ();
}


int main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  /*
  GlobalValue::Bind ("SimulatorImplementationType",
    StringValue ("ns3::RealtimeSimulatorImpl"));


  Config::SetDefault ("ns3::RealtimeSimulatorImpl::SynchronizationMode",  StringValue ("HardLimit") );

  Config::SetDefault ("ns3::RealtimeSimulatorImpl::HardLimit",  TimeValue ( Seconds ( 30.0 ) ) ) ;
*/

  NodeContainer nodes;
  nodes.Create (1);

  CreateFiles ();

  InternetStackHelper stack;
  stack.Install (nodes);

  DceManagerHelper dceManager;
  dceManager.Install (nodes);

  CcnClientHelper dce;
  ApplicationContainer apps, putter, getter;

  dce.SetStackSize (1 << 20);

  // Start the ccnd daemon
  dce.SetBinary ("ccnd");
  dce.ResetArguments ();
  dce.ResetEnvironment ();

  dce.AddEnvironment ("CCND_CAP", "50000");
  dce.AddEnvironment ("CCND_DEBUG", "-1");
  dce.AddEnvironment ("CCN_LOCAL_PORT", "");

  dce.AddEnvironment ("CCND_CAP", "");
  dce.AddEnvironment ("CCND_AUTOREG", "");
  dce.AddEnvironment ("CCND_LISTEN_ON", "");
  dce.AddEnvironment ("CCND_MTU", "");
  dce.AddEnvironment ("CCND_LOCAL_SOCKNAME", "");
  dce.AddEnvironment ("CCND_DATA_PAUSE_MICROSEC", "");
  dce.AddEnvironment ("CCND_KEYSTORE_DIRECTORY", "");

  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (1.0));

  // Send a little file : ccnsendchunks ccnx:/test_short_stuff/42 </tmp/100
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.SetBinary ("ccnsendchunks");
  dce.SetStdinFile ("/tmp/100");
  dce.AddFile ("/tmp/100","/tmp/100");
  dce.AddArgument ("ccnx:/test_short_stuff/42");
  dce.AddEnvironment ("HOME", "/root");

  putter = dce.Install (nodes.Get (0));
  putter.Start (Seconds (1.1));

  // Retrieve file : ccncatchunks ccnx:/test_short_stuff/42
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.SetBinary ("ccncatchunks");
  dce.SetStdinFile ("");
  dce.AddArgument ("ccnx:/test_short_stuff/42");
  dce.AddEnvironment ("HOME", "/root");

  getter = dce.Install (nodes.Get (0));
  getter.Start (Seconds (1.2));

  // Add-On: test ccndumpnames
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.SetBinary ("ccndumpnames");
  dce.SetStdinFile ("");
  dce.AddEnvironment ("HOME", "/root");

  getter = dce.Install (nodes.Get (0));
  getter.Start (Seconds (1.3));

  // Add-On: test ccnls
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.SetBinary ("ccnls");
  dce.SetStdinFile ("");
  dce.AddEnvironment ("HOME", "/root");
  dce.AddArgument ("/test_short_stuff/");

  getter = dce.Install (nodes.Get (0));
  getter.Start (Seconds (1.32));

  // Add-On: test ccnrm
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.SetBinary ("ccnrm");
  dce.SetStdinFile ("");
  dce.AddEnvironment ("HOME", "/root");
  dce.AddArgument ("/test_short_stuff/");

  getter = dce.Install (nodes.Get (0));
  getter.Start (Seconds (1.4));

  // Add-On: test ccnseqwriter
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.SetBinary ("ccnseqwriter");
  dce.SetStdinFile ("/tmp/README");
  dce.AddFile ("/tmp/README", "/tmp/README");
  dce.AddEnvironment ("HOME", "/root");
  dce.AddArgument ("/test-seq-writer/");

  getter = dce.Install (nodes.Get (0));
  getter.Start (Seconds (1.5));

  // Add-On: test ccncat
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.SetBinary ("ccncat");
  dce.SetStdinFile ("");
  dce.AddEnvironment ("HOME", "/root");
  dce.AddArgument ("/test-seq-writer/");

  getter = dce.Install (nodes.Get (0));
  getter.Start (Seconds (1.6));

  // Add-On: test ccnslurp
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.SetBinary ("ccnslurp");
  dce.SetStdinFile ("");
  dce.AddEnvironment ("HOME", "/root");
  dce.AddArgument ("/");

  getter = dce.Install (nodes.Get (0));
  getter.Start (Seconds (1.65));

  // Stop ccnd before simu end.
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.SetBinary ("ccndsmoketest");
  dce.SetStdinFile ("");
  dce.AddArgument ("kill");
  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (19.9));


  Simulator::Stop (Seconds (20.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
