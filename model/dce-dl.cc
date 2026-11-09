/* -*-	Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Hajime Tazaki
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
 * Author: Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#include "loader-factory.h"
#include "exec-utils.h"
#include "utils.h"
#include "process.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include <unistd.h>
#include <dlfcn.h>

NS_LOG_COMPONENT_DEFINE ("DceDlfcn");

using namespace ns3;

void *dce_dlopen(const char *filename, int flag)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << filename << flag);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  Loader *loader = current->process->loader;
  int errNo;
  std::string vpath = "";
  char *pvpath = seek_env ("PATH", current->process->originalEnvp);
  if (pvpath)
    {
      vpath = std::string (pvpath);
    }

  std::string exeFullPath = SearchExecFile (current->process->originalArgv[0], vpath, getuid (), getgid (), &errNo);

  void *handle = loader->Load (filename ? filename : exeFullPath, flag, true);
  if (handle == 0)
    {
      const char *error = dlerror ();
      current->process->dlError = error != 0 ? error
        : std::string (filename) + ": cannot open shared object file: No such file or directory";
      current->process->hasDlError = true;
    }
  return handle;
}

void *dce_dlsym(void *handle, const char *symbol)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << handle << symbol);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  Loader *loader = current->process->loader;
  void *address = loader->Lookup (handle, symbol);
  if (address == 0)
    {
      current->process->dlError = std::string ("undefined symbol: ") + symbol;
      current->process->hasDlError = true;
    }
  return address;
}

char *dce_dlerror (void)
{
  // the error of the last dlopen() or dlsym() which failed, once
  Thread *current = Current ();
  NS_ASSERT (current != 0);
  if (!current->process->hasDlError)
    {
      return 0;
    }
  current->process->hasDlError = false;
  return (char *) current->process->dlError.c_str ();
}

int dce_dlclose(void *handle)
{
    return 0;
}
