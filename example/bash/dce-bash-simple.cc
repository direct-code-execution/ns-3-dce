#include "ns3/network-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/dce-module.h"
#include <sys/types.h>
#include <sys/stat.h>

using namespace ns3;

/*
 * Test with bash version 4.1.0
 *
 *  CFLAGS="-g -fPIC" LDFLAGS=-pie CFLAGS_FOR_BUILD='-g -fPIC'   ./configure --without-bash-malloc
 *  make
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
  //dceManager.SetVirtualPath ("/bin:/user/furbani/home/local/bin/");
  dceManager.Install (nodes);

  mkdir ("files-0", 0700);
  mkdir ("files-0/tmp", 0700);
  mkdir ("files-0/home", 0700);
  mkdir ("files-0/home/dce", 0700);

  FILE *script = fopen("files-0/tmp/script.sh", "w");
//                  12345678901234567890 123456 7890123456789012345
  fprintf (script, "echo BEFORE $LINENO\nuname\necho AFTER: $LINENO");
  //  fprintf (script, "cat <$0\necho $$\necho $LINENO\necho Hello NS3..\nexit 0\n" );
  /*

  fprintf (script, "cd /tmp\necho Hello NS3 Virtual Machine\nwhich echo\necho $PATH $PWD\n");
  fprintf (script, "ls /tmp\ncd /tmp\nccnd-init-keystore-helper /tmp\n");
  */
  fclose (script);

  DceApplicationHelper dce;
  ApplicationContainer apps;

//  dce.SetBinary ("/bin/sh not here");
//  dce.SetBinary ("/libtest.so"); // TEMPOFUR
  dce.SetBinary ("/bin/sh");
  dce.SetStackSize (1<<20);
  dce.AddEnvironment ("PATH","/bin:/usr/local/ssl/bin");
  dce.AddEnvironment ("HOME","/home/dce");
 // dce.SetStdinFile ("/tmp/script.sh");
 // dce.SetStdinFile ("/tmp/ccnd-init-keystore-helper");
  dce.AddArgument("/tmp/ccnd-init-keystore-helper");
  dce.AddArgument("urbanos");
  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (4.0));

  Simulator::Stop (Seconds(1000100.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
