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

  return loader->Load (filename ? filename : exeFullPath, flag, true);
}

void *dce_dlsym(void *handle, const char *symbol)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << handle << symbol);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  Loader *loader = current->process->loader;
  return loader->Lookup (handle, symbol);
}

int dce_dlclose(void *handle)
{
    return 0;
}
