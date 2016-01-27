#include "ns3/log.h"
#include "ns3/nstime.h"
#include "dce-poll.h"
#include "sys/dce-select.h"
#include "wait-queue.h"
#include "file-usage.h"
#include "utils.h"
#include "dce-manager.h"
#include "process.h"
#include "errno.h"
#include <map>


NS_LOG_COMPONENT_DEFINE ("DcePollSelect");

using namespace ns3;

int dce_poll (struct pollfd *fds, nfds_t nfds, int timeout)
{
  int count = -1;
  int timed_out = 0;
  Time endtime;
  // key fd
  std::map <int, FileUsage*> toUnRef;
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fds << nfds << timeout);
  PollTable *table = new PollTable ();
  PollTable *currentTable = table;

  NS_ASSERT (current != 0);

  if (0 == timeout)
    {
      currentTable = 0;
      timed_out = 1;
    }
  else if (timeout > 0)
    {
      endtime = Now () + MilliSeconds (timeout);
    }

  for (uint32_t i = 0; i < nfds; ++i)
    {
      // initialize all outgoing events.
      fds[i].revents = 0;
    }
  while (true)
    {
      int validFd = 0;
      count = 0;
      for (uint32_t i = 0; i < nfds; ++i)
        {
          if (CheckFdExists (current->process, fds[i].fd, true))
            {
              validFd++;
              UnixFd *unixFd = 0;
              FileUsage *fu = current->process->openFiles[fds[i].fd];

              if (currentTable)
                {
                  unixFd = fu->GetFileInc ();
                  toUnRef[fds[i].fd] = fu;
                  currentTable->SetEventMask (fds[i].events | POLLERR | POLLHUP);
                }
              else
                {
                  unixFd = fu->GetFile ();
                }

              int mask = unixFd->Poll (currentTable);

              mask &= (fds[i].events | POLLERR | POLLHUP);
              fds[i].revents = mask;
              if (mask)
                {
                  count++;
                  currentTable = 0;
                }
            }
        }
      currentTable = 0; // Register only first time.

      if (count || timed_out)
        {
          break;
        }

      if (timeout != 0)
        {
          if (timeout < 0)
            {
              current->pollTable = table;
              table->Wait (Seconds (0));
              current->pollTable = 0;
            }
          else
            {
              Time diff = endtime - Now ();
              if (diff.IsStrictlyPositive ())
                {
                  current->pollTable = table;
                  table->Wait (diff);
                  current->pollTable = 0;
                }
              else
                {
                  break;
                }
            }
        }
    }

  table->FreeWait ();
  delete (table);

  for (std::map <int, FileUsage*>::iterator i = toUnRef.begin ();
       i != toUnRef.end (); ++i)
    {
      i->second->DecUsage ();
    }

  // Try to break infinite loop in poll with a 0 timeout !
  if ((0 == count) && (0 == timeout))
    {
      UtilsAdvanceTime (current);
    }

  return count;
}

int dce_select (int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
                struct timeval *timeout)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << nfds << timeout);
  NS_ASSERT (current != 0);
  std::map <int, int> eventByFd;

  if (nfds == -1)
    {
      current->err = EINVAL;
      return -1;
    }
  if (readfds == 0 && writefds == 0 && exceptfds == 0)
    {
      current->err = EINVAL;
      return -1;
    }
  if (timeout)
    {
      if (timeout->tv_sec < 0 || timeout->tv_usec < 0)
        {
          current->err = EINVAL;
          return -1;
        }
    }
  for (int fd = 0; fd < nfds; fd++)
    {
      int event = 0;

      if (readfds != 0 && FD_ISSET (fd, readfds))
        {
          event |= POLLIN;
        }
      if (writefds != 0 &&  FD_ISSET (fd, writefds))
        {
          event |= POLLOUT;
        }
      if (exceptfds != 0 && FD_ISSET (fd, exceptfds))
        {
          event |= POLLPRI;
        }

      if (event)
        {
          if (!CheckFdExists (current->process, fd, true))
            {
              current->err = EBADF;
              return -1;
            }
          eventByFd[fd] = event;
        }
    }
  nfds = eventByFd.size ();

  // select(2):
  // Some  code  calls  select() with all three sets empty, nfds zero, and a
  // non-NULL timeout as a fairly portable way to sleep with subsecond
  // precision.
  // 130825: this condition will be passed by dce_poll ()

  struct pollfd pollFd[nfds];
  int j = 0;

  for (std::map<int, int>::iterator i = eventByFd.begin (); i != eventByFd.end (); ++i)
    {
      pollFd[j].events = (*i).second;
      pollFd[j].fd = (*i).first;
      pollFd[j++].revents = 0;
    }

  int pollTo = -1;

  if (timeout)
    {
      pollTo = timeout->tv_sec * 1000 + timeout->tv_usec / 1000;
    }

  int pollRet = dce_poll (pollFd, nfds, pollTo);

  if (readfds)
    {
      FD_ZERO (readfds);
    }
  if (writefds)
    {
      FD_ZERO (writefds);
    }
  if (exceptfds)
    {
      FD_ZERO (exceptfds);
    }
    
  if (pollRet > 0)
    {
      pollRet = 0;
      for (j = 0; j < nfds; j++)
        {
          if (readfds && ((POLLIN & pollFd[j].revents) || (POLLHUP & pollFd[j].revents)
                          || (POLLERR & pollFd[j].revents)))
            {
              FD_SET (pollFd[j].fd, readfds);
              pollRet++;
            }
          if (writefds && (POLLOUT & pollFd[j].revents))
            {
              FD_SET (pollFd[j].fd, writefds);
              pollRet++;
            }
          if (exceptfds && (POLLPRI & pollFd[j].revents))
            {
              FD_SET (pollFd[j].fd, exceptfds);
              pollRet++;
            }
        }
    }
  return pollRet;
}
