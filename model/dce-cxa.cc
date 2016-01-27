#include "dce-cxa.h"
#include "utils.h"
#include "process.h"
#include "ns3/assert.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("DceCxa");

using namespace ns3;

int
dce___cxa_atexit (void (*func)(void *), void *arg, void *d)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << func << arg << d);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  struct AtExitHandler handler;
  handler.type = AtExitHandler::CXA;
  handler.value.cxa.fn = func;
  handler.value.cxa.arg = arg;
  handler.value.cxa.d = d;
  current->process->atExitHandlers.push_back (handler);
  return 0;
}
void dce___cxa_finalize (void *d)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << d);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  // from back to front to ensure that we process all this in the right order
  // which is from last registered to first registered.
  // and that we handle the case where one of the atexit handlers
  // recursively calls atexit or __cxa_atexit.
  while (!current->process->atExitHandlers.empty ())
    {
      struct AtExitHandler handler = current->process->atExitHandlers.back ();
      current->process->atExitHandlers.pop_back ();
      switch (handler.type)
        {
        case AtExitHandler::CXA:
          if (d == 0 || handler.value.cxa.d == d)
            {
              handler.value.cxa.fn (handler.value.cxa.arg);
            }
          break;
        case AtExitHandler::NORMAL:
          if (d == 0)
            {
              handler.value.normal ();
            }
          break;
        }
    }
}
