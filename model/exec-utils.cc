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
#include <iostream>
#include <sstream>
#include <string.h>
#include <vector>
#include "exec-utils.h"
#include "utils.h"
#include "dce-manager.h"
#include "process.h"
#include "dce-unistd.h"
#include "errno.h"

namespace ns3 {

struct ExeCriteria
{
  uid_t uid;
  gid_t gid;
  int *errNo;
};

bool CanonizePath (std::string path, bool ChRootProtected, std::ostringstream &result)
{
  std::vector <std::string> profs;
  std::ostringstream cur;
  int level = 0;
  const char *crsr = path.c_str ();
  int state = 0;
  bool SlashStart = ('/' == *crsr);

  do
    {
      switch (state)
        {
        case 0:
          {
            switch (*crsr)
              {
              case '.':
                {
                  state = 1;
                }
                break;
              case 0:
              case '/':
                break;
              default:
                {
                  cur << *crsr;
                  state = 2;
                }
                break;
              }
          }
          break;
        case 1: // .
          {
            switch (*crsr)
              {
              case '.':
                {
                  state = 3;
                }
                break;
              case 0:
              case '/':
                {
                  state = 0;
                }
                break;
              default:
                {
                  cur << '.' << *crsr;
                  state = 2;
                }
                break;
              }
          }
          break;
        case 2:  // other
          {
            if (('/' == *crsr)||(!*crsr))
              {
                state = 0;
                level++;
                std::string e = cur.str ();
                profs.push_back (e);
                cur.clear ();
                cur.str ("");
              }
            else
              {
                cur << *crsr;
              }
          }
          break;
        case 3: // ..
          {
            switch (*crsr)
              {
              case 0:
              case '/':
                {
                  level--;
                  if (ChRootProtected && !SlashStart && (level < 0))
                    {
                      return false;
                    }
                  if (level >= 0)
                    {
                      profs.pop_back ();
                    }
                  else
                    {
                      profs.push_back ("..");
                    }
                  state = 0;
                }
                break;
              default:
                {
                  cur << '.' << '.' << *crsr;
                  state = 2;
                }
                break;
              }
          }
          break;
        default:
          return false;
        }
      if (*crsr)
        {
          crsr++;
        }
      else
        {
          break;
        }
    }
  while (true);

  bool first = true;
  if (profs.size () > 0)
    {
      for (std::vector <std::string> :: iterator i = profs.begin () ; i != profs.end () ; ++i)
        {
          if (!first || SlashStart)
            {
              result << '/' ;
            }
          first = false;
          result << *i ;
        }
    }
  else
    {
      if (SlashStart)
        {
          result << '/' ;
        }
      else
        {
          result << '.' ;
        }
    }

  return true;
}
SearchPath::SearchPath (std::string basepath, std::string paths, bool virt) : m_basePath (basepath),
                                                                              m_virtual (virt)
{
  bool prot = (m_basePath.length () > 0);
  std::list<std::string> p = Split (paths, ":");
  for (std::list<std::string>::const_iterator i = p.begin (); i != p.end (); i++)
    {
      std::ostringstream cano;
      if (CanonizePath (*i, prot, cano))
        {
          std::string e = cano.str ();
          if (e.length () > 0)
            {
              m_paths.push_back (e);
            }
        }
    }
}
std::string
SearchPath::SeekFile (std::string file, std::string cwd, void *userData, bool (*checker)(std::string, void*))
{
  for (std::vector <std::string>::const_iterator i = m_paths.begin (); i != m_paths.end (); ++i)
    {
      std::string p = *i;
      std::string pp;
      if (*p.c_str () == '/')
        {
          if (m_virtual)
            {
              if (p.length () > 0)
                {
                  pp = m_basePath + "/" + p + "/" + file;
                }
              else
                {
                  pp = m_basePath + "/" + file;
                }
            }
          else
            {
              pp = p + "/" + file;
            }
        }
      else
        {
          if (m_virtual)
            {
              if ((cwd.length () > 0) && (p.length () > 0))
                {
                  pp = m_basePath + "/" + cwd + "/" + p + "/" + file;
                }
              else
              if (cwd.length () > 0)
                {
                  pp = m_basePath + "/" + cwd + "/" + file;
                }
              else
                {
                  pp = m_basePath + "/" + file;
                }
            }
          else
            {
              pp = p + "/" + file;
            }
        }
      std::ostringstream cano;
      CanonizePath (pp, false, cano);
      pp = cano.str ();
      if ((pp.length () > 0) &&  checker (pp, userData))
        {
          return pp;
        }
    }
  return "";
}
std::string
SearchAbsoluteFile (std::string file, std::string vroot, std::string altRoots, void *userData,
                    bool (*checker)(std::string, void*))
{
  SearchPath p1 (vroot, ".", true);
  std::string res = p1.SeekFile (file, "", userData, checker);
  if (res.length () > 0)
    {
      return res;
    }
  else
    {
      SearchPath p2 ("", altRoots, false);
      return p2.SeekFile (file, "", userData, checker);
    }
}
std::string
SearchRelativeFile (std::string file, std::string vroot, std::string cwd,
                    void *userData, bool (*checker)(std::string, void*))
{
  SearchPath p1 (vroot, cwd, true);
  return p1.SeekFile (file, "", userData, checker);
}
std::string
SearchSimpleFile (std::string file,
                  std::string vroot,
                  std::string vpath,
                  std::string cwd,
                  std::string dcepath,
                  void *userData,
                  bool (*checker)(std::string, void*))
{
  SearchPath p1 (vroot, vpath, true);
  std::string res = p1.SeekFile (file, cwd, userData, checker);
  if (res.length () > 0)
    {
      return res;
    }
  else
    {
      SearchPath p2 ("", dcepath, false);
      return p2.SeekFile (file,"", userData, checker);
    }
}
std::string
SearchFile (std::string file,
            std::string vroot,
            std::string vpath,
            std::string dcepath,
            std::string cwd,
            std::string altRoots,
            void *userData,
            bool (*checker)(std::string, void*))
{
  if (*file.c_str () == '/')
    {
      return SearchAbsoluteFile (file, vroot, altRoots, userData, checker);
    }
  int idx =  file.find ('/',0);
  if (idx >= 0)
    {
      return SearchRelativeFile (file, vroot, cwd, userData, checker);
    }
  return SearchSimpleFile (file, vroot, vpath, cwd, dcepath, userData, checker);
}
std::string
SearchFile (std::string file,
            std::string vroot,
            std::string cwd,
            std::string altRoots,
            void *userData,
            bool (*checker)(std::string, void*))
{
  if (*file.c_str () == '/')
    {
      return SearchAbsoluteFile (file, vroot, altRoots, userData, checker);
    }
  return SearchRelativeFile (file, vroot, cwd, userData, checker);
}
bool
CheckFileExe (std::string file, void *params)
{
  struct stat st;
  struct ExeCriteria *criteria = (struct ExeCriteria *) params;

  if (0 != ::stat (file.c_str (), &st))
    {
      return false;
    }
  if (criteria)
    {
      uid_t uid = criteria->uid;
      gid_t gid = criteria->gid;
      if (((uid) && CheckExeMode (&st, uid, gid)) || (!uid))
        {
          return true;
        }
      if (criteria->errNo)
        {
          *criteria->errNo = ENOEXEC;
        }
      return false;
    }
  return true;
}
std::string
SearchExecFile (std::string file, std::string vpath, uid_t uid, gid_t gid, int *errNo)
{
  std::string vroot = UtilsGetRealFilePath ("/");
  Thread *current = Current ();
  DceManager *manager = current->process->manager;
  std::string dcepath = "";
  std::string cwd = dce_get_current_dir_name ();
  std::string altRoots = "";
  char *c = getenv ("DCE_PATH");
  struct ExeCriteria userData;
  if (errNo)
    {
      *errNo = ENOENT;
    }
  if (vpath.length () == 0)
    {
      if (manager->GetVirtualPath ().length () > 0)
        {
          vpath = manager->GetVirtualPath ();
        }
    }
  else
    {
      if (manager->GetVirtualPath ().length () > 0)
        {
          vpath = vpath + ':' + manager->GetVirtualPath ();
        }
    }
  if (c)
    {
      dcepath = c;
    }
  c = getenv ("DCE_ROOT");
  if (c)
    {
      altRoots = c;
    }
  userData.uid = uid;
  userData.gid = gid;
  userData.errNo = errNo;

  return SearchFile (file, vroot, vpath, dcepath, cwd, altRoots, &userData, CheckFileExe);
}
std::string
SearchExecFile (std::string file, uid_t uid, gid_t gid, int *errNo)
{
  std::string vroot = UtilsGetRealFilePath ("/");
  std::string cwd = dce_get_current_dir_name ();
  std::string altRoots = "";
  struct ExeCriteria userData;
  if (errNo)
    {
      *errNo = ENOENT;
    }
  char *c = getenv ("DCE_ROOT");
  if (c)
    {
      altRoots = c;
    }
  userData.uid = uid;
  userData.gid = gid;
  userData.errNo = errNo;

  return SearchFile (file, vroot,  cwd, altRoots, &userData, CheckFileExe);
}
// Search using only a real path within a real environment variable
std::string
SearchExecFile (std::string envVar, std::string file, int *errNo)
{
  char *c = getenv (envVar.c_str ());
  std::string dcepath = "";
  struct ExeCriteria userData;

  if (c)
    {
      dcepath = c;
    }
  userData.uid = 0;
  userData.gid = 0;
  userData.errNo = 0;

  return SearchFile (file, "", "", dcepath, "", "", &userData, CheckFileExe);
}

}
