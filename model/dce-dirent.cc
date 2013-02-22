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
#include "dce-dirent.h"
#include "dce-fcntl.h"
#include "dce-unistd.h"
#include "unix-fd.h"
#include "unix-file-fd.h"
#include "file-usage.h"
#include "process.h"
#include "utils.h"
#include "unix-fd.h"
#include "ns3/log.h"
#include "errno.h"
#include "dce-stdlib.h"
#include <string.h>

NS_LOG_COMPONENT_DEFINE ("DceDirent");

struct my__dirstream
{
  int fd; /* File descriptor.  */
};

namespace ns3 {
void
remove_dir (DIR *d, Thread *current)
{
  bool found = false;
  for (std::vector<DIR*>::iterator  i = current->process->openDirs.begin ();
       i != current->process->openDirs.end (); ++i)
    {
      if (*i == d)
        {
          current->process->openDirs.erase (i);
          found = true;
          break;
        }
    }
  if (!found)
    {
      // not found
      NS_FATAL_ERROR ("invalid DIR * closed=" << d);
    }
}

int dce_internalClosedir (DIR *dirp, struct Thread *cur)
{
  struct my__dirstream *ds = (struct my__dirstream *) dirp;
  int saveFd = -1;

  if (cur)
    {
      saveFd = ds->fd;
      if (saveFd >= 0)
        {
          cur->err = EBADF;
          return -1;
        }
      int realFd = getRealFd (-saveFd, cur);
      if (realFd < 0)
        {
          ds->fd = saveFd;
          cur->err = EBADF;
          return 0;
        }
      ds->fd = realFd;
    }
  else
    {
      ds->fd = -1;
    }
  int ret = closedir (dirp);
  if (cur)
    {
      if (ret == 0)
        {
          dce_close (-saveFd);
          ds->fd = saveFd;
          remove_dir (dirp, cur);
        }
      else
        {
          cur->err = errno;
        }
    }
  return ret;
}
}

using namespace ns3;

DIR * dce_opendir (const char *name)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << name);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  int fd = dce_open (name, O_DIRECTORY, 0);
  if (fd == -1)
    {
      current->err = errno;
      return 0;
    }
  DIR *res = dce_fdopendir (fd);
  if (res == 0)
    {
      dce_close (fd);
    }
  return res;
}
DIR * dce_fdopendir (int fd)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << fd);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();

  // no need to create or truncate. Just need to seek if needed.
  DIR *res = opendir ("/");
  if (res == 0)
    {
      current->err = errno;
      return 0;
    }
  struct my__dirstream *ds = (struct my__dirstream *) res;
  close (ds->fd);
  ds->fd = -fd;
  current->process->openDirs.push_back (res);

  return res;
}

struct dirent * dce_readdir (DIR *dirp)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  struct my__dirstream *ds = (struct my__dirstream *) dirp;
  int saveFd = ds->fd;
  if (saveFd >= 0)
    {
      current->err = EBADF;
      return 0;
    }
  int realFd = getRealFd (-saveFd, current);
  if (realFd < 0)
    {
      ds->fd = saveFd;
      current->err = EBADF;
      return 0;
    }
  ds->fd = realFd;
  struct dirent *ret = readdir (dirp);
  ds->fd = saveFd;

  return ret;
}

int dce_readdir_r (DIR *dirp, struct dirent *entry, struct dirent **result)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  struct my__dirstream *ds = (struct my__dirstream *) dirp;

  int saveFd = ds->fd;
  if (saveFd >= 0)
    {
      current->err = EBADF;
      return -1;
    }
  int realFd = getRealFd (-saveFd, current);
  if (realFd < 0)
    {
      ds->fd = saveFd;
      current->err = EBADF;
      return -1;
    }
  ds->fd = realFd;
  int ret = readdir_r (dirp, entry, result);
  ds->fd = saveFd;

  return ret;
}
int dce_closedir (DIR *dirp)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  return dce_internalClosedir (dirp, current);
}
int dce_dirfd (DIR *dirp)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  current->err = ENOTSUP;
  return -1;
}
void dce_rewinddir (DIR *dirp)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  struct my__dirstream *ds = (struct my__dirstream *) dirp;

  int saveFd = ds->fd;
  if (saveFd >= 0)
    {
      return;
    }
  int realFd = getRealFd (-saveFd, current);
  if (realFd < 0)
    {
      ds->fd = saveFd;
      return;
    }
  ds->fd = realFd;
  rewinddir (dirp);
  ds->fd = saveFd;
}
int dce_scandir (const char *dirp, struct dirent ***namelist,
                 int (*filter)(const struct dirent *),
                 int (*compar)(const struct dirent **, const struct dirent **))
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);

  std::string vPath = UtilsGetRealFilePath (std::string (dirp));

  struct dirent **nl = 0;
  int ret = scandir (vPath.c_str (), &nl, filter, compar);

  if ((ret > 0) && nl)
    {
      // Realloc !
      struct dirent **res = 0;
      size_t direntMiniSize = 0;

      if (namelist)
        {
          res = (struct dirent **) dce_malloc (sizeof (struct dirent *) * ret);
        }
      {
        struct dirent tmp;
        direntMiniSize = sizeof (tmp.d_ino) + sizeof (tmp.d_off)
          + sizeof (tmp.d_reclen) + sizeof (tmp.d_type);
      }
      for (int i = 0; i < ret ; i++)
        {
          struct dirent *src =  nl [i];
          struct dirent *copy = 0;
          if (namelist)
            {
              size_t direntSize = direntMiniSize + _D_ALLOC_NAMLEN (src);
              copy = (struct dirent *) dce_malloc (direntSize);
              memcpy (copy, src, direntSize);
              res [i] = copy;
            }
          free (src);
        }
      free (nl);
      if (namelist)
        {
          *namelist = res;
        }
    }

  return ret;
}
