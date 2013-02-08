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
 */
#include "dce-fcntl.h"
#include "sys/dce-stat.h"
#include "process.h"
#include "utils.h"
#include "ns3/log.h"
#include "errno.h"
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

NS_LOG_COMPONENT_DEFINE ("DceAt");

using namespace ns3;

int dce___fxstatat (int ver, int fd, const char *pathname, struct stat *buf, int flag)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << pathname << buf);
  NS_ASSERT (current != 0);
  int retval = -1;

  if ((0 == pathname) || (0 == buf))
    {
      current->err = EFAULT;
      return -1;
    }
  if (std::string (pathname) == "")
    {
      current->err = ENOENT;
      return -1;
    }
  if (fd != AT_FDCWD && pathname[0] != '/')
    {
      int realFd = getRealFd (fd, current);

      if (realFd < 0)
        {
          current->err = EBADF;
          return -1;
        }
      retval = ::__fxstatat (ver, realFd, pathname, buf, flag);
    }
  else
    {
      std::string path = UtilsGetCurrentDirName () + "/" +  UtilsGetRealFilePath (pathname);
      retval = ::__fxstatat (ver, fd, path.c_str (), buf, flag);
    }
  if (retval == -1)
    {
      current->err = errno;
      return -1;
    }
  return retval;
}
void unlink_notify (std::string fullpath);
int dce_unlinkat (int fd, const char *pathname, int flags)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << pathname);
  NS_ASSERT (current != 0);
  int retval = -1;
  std::string realpath = "";

  if (0 == pathname)
    {
      current->err = EFAULT;
      return -1;
    }
  if (std::string (pathname) == "")
    {
      current->err = ENOENT;
      return -1;
    }
  if (fd != AT_FDCWD && pathname[0] != '/')
    {
      int realFd = getRealFd (fd, current);

      if (realFd < 0)
        {
          current->err = EBADF;
          return -1;
        }
      retval = ::unlinkat (realFd, pathname, flags);
      if (retval == 0)
        {
          realpath = PathOfFd (realFd);
        }
    }
  else
    {
      realpath = UtilsGetCurrentDirName () + "/" + UtilsGetRealFilePath (pathname);
      retval = ::unlinkat (fd, realpath.c_str (), flags);
    }
  if (retval == -1)
    {
      current->err = errno;
      return -1;
    }

  if (retval == 0)
    {
      unlink_notify (realpath);
    }

  return retval;
}
