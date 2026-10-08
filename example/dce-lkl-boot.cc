/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Boot an LKL (Linux Kernel Library) kernel on each node and check that
 * kernel time follows the simulation clock: a 10 s nanosleep inside the
 * kernel must take 10 s of simulated time and almost no wall-clock time.
 */
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/dce-module.h"
#include "ns3/lkl-kernel.h"
#include "ns3/task-manager.h"
#include <lkl/asm/unistd.h>
#include <chrono>
#include <cstring>
#include <iostream>

using namespace ns3;

static int g_failures = 0;
static uint32_t g_completed = 0;

static void
CheckNode (Ptr<Node> node)
{
  Ptr<LklKernel> kernel = node->GetObject<LklKernel> ();
  kernel->Boot ();

  char uts[6][65];
  memset (uts, 0, sizeof (uts));
  long params[6] = { (long)uts, 0, 0, 0, 0, 0 };
  long ret = kernel->Syscall (__lkl__NR_uname, params);
  std::cout << "node " << node->GetId () << ": uname=" << ret << " "
            << uts[0] << " " << uts[2] << std::endl;

  struct
  {
    long long tv_sec;
    long long tv_nsec;
  } duration = { 10, 0 };
  Time before = Simulator::Now ();
  auto wallBefore = std::chrono::steady_clock::now ();
  long sleepParams[6] = { (long)&duration, 0, 0, 0, 0, 0 };
  ret = kernel->Syscall (__lkl__NR_nanosleep, sleepParams);
  Time slept = Simulator::Now () - before;
  double wall = std::chrono::duration<double> (std::chrono::steady_clock::now () - wallBefore).count ();
  std::cout << "node " << node->GetId () << ": nanosleep(10s)=" << ret
            << " simulated " << slept.GetNanoSeconds () << " ns, wall " << wall << " s" << std::endl;
  // Linux adds the task's timer slack (50 us by default) to the sleep.
  if (ret != 0 || slept < Seconds (10) || slept > Seconds (10) + MilliSeconds (1))
    {
      g_failures++;
    }
  g_completed++;
}

static void
CheckTask (void *context)
{
  CheckNode ((Node *)context);
  TaskManager::Current ()->Exit ();
}

static void
StartCheck (Ptr<Node> node)
{
  node->GetObject<TaskManager> ()->Start (&CheckTask, PeekPointer (node), 1 << 18);
}

int
main (int argc, char *argv[])
{
  uint32_t nNodes = 2;
  CommandLine cmd;
  cmd.AddValue ("nodes", "Number of nodes", nNodes);
  cmd.Parse (argc, argv);

  NodeContainer nodes;
  nodes.Create (nNodes);
  DceManagerHelper dceManager;
  dceManager.Install (nodes);
  for (uint32_t i = 0; i < nNodes; i++)
    {
      nodes.Get (i)->AggregateObject (CreateObject<LklKernel> ());
      // Start the nodes at different times so that their kernels interleave.
      Simulator::ScheduleWithContext (i, Seconds (1.0 + 0.5 * i), &StartCheck, nodes.Get (i));
    }

  Simulator::Stop (Seconds (30));
  Simulator::Run ();
  Simulator::Destroy ();

  bool ok = g_failures == 0 && g_completed == nNodes;
  std::cout << (ok ? "OK" : "FAILED") << " (" << g_completed << "/" << nNodes << " nodes completed)" << std::endl;
  return ok ? 0 : 1;
}
