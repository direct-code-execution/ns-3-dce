/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012 INRIA
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
 * Author: Fr辿d辿ric Urbani
 *         Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#include "ns3/network-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/dce-module.h"
#include <sys/types.h>
#include <sys/stat.h>

using namespace ns3;

/*
 * Tested with bash version 4.1.0
 *
 */

int main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  NodeContainer nodes;
  nodes.Create (1);

  InternetStackHelper stack;
  stack.Install (nodes);

  DceManagerHelper dceManager;
  dceManager.Install (nodes);

  mkdir ("files-0", 0700);
  mkdir ("files-0/tmp", 0700);
  mkdir ("files-0/home", 0700);
  mkdir ("files-0/home/dce", 0700);

  FILE *script = fopen ("files-0/tmp/script.sh", "w");
  fprintf (script, "echo BEFORE $LINENO\ncd /tmp\necho $PWD\necho AFTER: $LINENO");
  fclose (script);

  DceApplicationHelper dce;
  ApplicationContainer apps;

  dce.SetBinary ("bash");
  dce.SetStackSize (1 << 20);
  dce.AddEnvironment ("PATH","/bin:/usr/local/ssl/bin");
  dce.AddEnvironment ("HOME","/home/dce");
  dce.AddArgument ("/tmp/script.sh");
  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (4.0));

  Simulator::Stop (Seconds (10.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
