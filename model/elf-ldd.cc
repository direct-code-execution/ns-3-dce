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
 * Author: Frederic Urbani <frederic.urbani@inria.fr>
 *
 */
#include "elf-ldd.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/mman.h>
#include <elf.h>
#include <link.h>
#include <set>
#include <map>
#include <vector>
#include <utility>
#include <string>
#include <iostream>

namespace ns3 {
using namespace std;

NS_LOG_COMPONENT_DEFINE ("ElfLdd");

class SharedLibrary
{
public:
  SharedLibrary (string sName, string fullName);
  void AddLibrary (string l);
  void AddPath (string p);
  void SetSoName (string s);
  void SetArch32 ();
  void SetArch64 ();
  string GetName (bool full) const;
  bool GetArch32 () const;
  bool GetArch64 () const;
  set <pair<string,string> > GetLibs () const;
  vector <string> GetPath () const;
  string GetSoname () const;
  bool SearchLibs ();
  bool CalcDepth (int max, map <string, SharedLibrary*> &mapScanned);
  int GetDepth () const;

private:
  // ELF file name
  const string m_fullName;
  // Short Name
  const string m_sName;
  // Set of library found in elf file short
  set <pair<string,string> > m_libs;
  // vector of paths found in elf file (in DT_PATH and DT_RUNPATH) each entry is checked before (exists on this computer).
  vector <string> m_path;
  // DT_SONAME seen
  string m_soname;
  bool m_arch32;
  bool m_arch64;
  int m_depth;
};

SharedLibrary::SharedLibrary (string s,string f) : m_fullName (f),
                                                   m_sName (s),
                                                   m_arch32 (0),
                                                   m_arch64 (0),
                                                   m_depth (-1)
{
}

void
SharedLibrary::AddLibrary (string l)
{
  m_libs.insert (make_pair (l, ""));
}

void
SharedLibrary::AddPath (string p)
{
  char *start = (char*) p.c_str ();
  char *crsr = start;

  while (*crsr)
    {
      char *f = crsr;
      char prec = 0;

      while (*crsr && (*crsr != ':'))
        {
          prec = *crsr;
          crsr++;
        }
      char *l = crsr;

      if (l > f)
        {
          string res = string (f, l - f);
          if ((prec != 0) && (prec != '/'))
            {
              res += '/';
            }
          if (res.length () > 0)
            {
              DIR *d = opendir (res.c_str ());

              if (d)
                {
                  m_path.push_back (res);
                  closedir (d);
                }
            }
        }
      if (*crsr)
        {
          crsr++;
        }
    }
}

void
SharedLibrary::SetSoName (string s)
{
  m_soname = s;
}
set <pair<string,string> >
SharedLibrary::GetLibs () const
{
  return m_libs;
}
vector <string>
SharedLibrary::GetPath () const
{
  return m_path;
}
string
SharedLibrary::GetSoname () const
{
  return m_soname;
}
void
SharedLibrary::SetArch32 ()
{
  m_arch32 = 1;
}
void
SharedLibrary::SetArch64 ()
{
  m_arch64 = 1;
}
bool
SharedLibrary::GetArch32 () const
{
  return m_arch32;
}
bool SharedLibrary::GetArch64 () const
{
  return m_arch64;
}
string
SharedLibrary::GetName (bool full) const
{
  if (full)
    {
      return m_fullName;
    }
  else
    {
      return m_sName;
    }
}
bool
SharedLibrary::SearchLibs ()
{
  uint32_t count = 0;
  set<pair<string,string> > n;

  for (set<pair<string,string> >::const_iterator i = m_libs.begin (); i != m_libs.end (); ++i)
    {
      bool f = 0;
      pair <string,string> p = *i;

      for (vector<string>::const_iterator j = m_path.begin (); j != m_path.end (); ++j)
        {
          string fullPath = *j + p.first;
          struct stat st;

          if (!stat (fullPath.c_str (), &st))
            {
              p.second = fullPath;
              n.insert (p);
              count++;
              f = 1;
              break;
            }
        }
      if (!f)
        {
          NS_LOG_DEBUG (p.first << " NOT FOUND");
        }
    }
  if (m_libs.size () == count)
    {
      m_libs = n;
      return 1;
    }
  return 0;
}
bool
SharedLibrary::CalcDepth (int max, map <string, SharedLibrary*> &mapScanned)
{
  if (m_depth >= 0)
    {
      return 1;
    }
  if (max < 0)
    {
      return 0;
    }
  if (m_libs.size () == 0)
    {
      m_depth = 0;
      return 1;
    }
  int calc = 0;
  for (set<pair<string,string> >::const_iterator i = m_libs.begin (); i != m_libs.end (); ++i)
    {
      if (!mapScanned[(*i).second]->CalcDepth (max - 1, mapScanned))
        {
          return 0;
        }
      if (mapScanned[(*i).second]->GetDepth () > calc - 1)
        {
          calc = mapScanned[(*i).second]->GetDepth () + 1;
        }
    }
  m_depth = calc;
  return 1;
}
int
SharedLibrary::GetDepth () const
{
  return m_depth;
}
// Convert a virtual address to an offset within the file
ElfW (Addr) vaddr_2_foffset (const ElfW (Ehdr) * he, const ElfW (Phdr) * pr, const ElfW (Addr) vma)
{
  for (int p = 0; p < he->e_phnum; p++)
    {
      if (pr[p].p_type == PT_LOAD)
        {
          if ((vma >=  pr[p].p_vaddr))
            {
              return vma - pr[p].p_vaddr + pr[p].p_offset;
            }
        }
    }

  return vma;
}
SharedLibrary*
ElfLdd::ExtractLibraries (std::string sName, std::string fullPath)
{
  NS_LOG_FUNCTION (sName << fullPath);
  int fd = open (fullPath.c_str (), O_RDONLY);
  if (fd == -1)
    {
      NS_LOG_ERROR (fullPath << ": unable to open file errno: " << errno);
      return 0;
    }
  struct stat st;
  int retval = fstat (fd, &st);
  if (retval)
    {
      NS_LOG_ERROR (fullPath << ": unable to fstat file errno: " << errno);
      return 0;
    }
  uint64_t size = st.st_size;
  uint8_t *buffer = (uint8_t *) mmap (0, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (((void*)-1) == buffer)
    {
      NS_LOG_ERROR (fullPath << ": unable to mmap file errno: " << errno);
      return 0;
    }
  close (fd);
  SharedLibrary *res = new SharedLibrary (sName, fullPath);
  const ElfW (Ehdr) * header = (ElfW (Ehdr) *)buffer;
  if (header->e_ident [EI_CLASS] == ELFCLASS64)
    {
      res->SetArch64 ();
    }
  if (header->e_ident [EI_CLASS] == ELFCLASS32)
    {
      res->SetArch32 ();
    }
  const ElfW (Phdr) * programTable = (ElfW (Phdr) *)(buffer + header->e_phoff);
  const ElfW (Shdr) * sectionTable = (ElfW (Shdr) *)(buffer + header->e_shoff);
  for (int s = 0; s < header->e_shnum ; s++)
    {
      if (SHT_DYNAMIC == sectionTable [s].sh_type)
        {
          int n = 0;
          ElfW (Dyn) * dynamics = (ElfW (Dyn) *)(buffer + sectionTable [s].sh_offset);
          bool turn = 1;
          ElfW (Addr) dt_strtab = 0;
          do
            {
              if (dynamics [n].d_tag == DT_STRTAB)
                {
                  turn = 0;
                  dt_strtab = vaddr_2_foffset (header, programTable, dynamics [n].d_un.d_ptr);
                }
              if (dynamics [n].d_tag == DT_NULL)
                {
                  turn = 0;
                }
              n++;
            }
          while (turn);
          n = 0;
          turn = 1;
          do
            {
              switch (dynamics [n].d_tag)
                {
                case DT_NULL:
                  {
                    turn = 0;
                  }
                  break;

                case DT_NEEDED:
                  {
                    res->AddLibrary (string ((char*)(buffer + dt_strtab + dynamics [n].d_un.d_ptr)));
                  }
                  break;

                case DT_RPATH:
                case DT_RUNPATH:
                  {
                    res->AddPath (string ((char*)(buffer + dt_strtab + dynamics [n].d_un.d_ptr)));
                  }
                  break;

                case DT_SONAME:
                  {
                    res->SetSoName (string ((char*)(buffer + dt_strtab + dynamics [n].d_un.d_ptr)));
                  }
                  break;
                }
              n++;
            }
          while (turn);
        }
    }
  retval = ::munmap (buffer, size);

  return res;
}

void
ElfLdd::Loop (std::string s, std::string f)
{
  set<pair<string,string> > todo;
  set<string> done;
  map <string, SharedLibrary*> mapScanned;
  todo.insert (make_pair (s,f));
  vector<SharedLibrary*> toFree;

  while (todo.size () > 0)
    {
      const pair<string,string> name = *(todo.begin ());
      todo.erase (name);
      SharedLibrary* l =  ExtractLibraries (name.first, name.second);
      if (l)
        {
          toFree.push_back (l);
          if (getenv ("LD_LIBRARY_PATH"))
            {
              l->AddPath (getenv ("LD_LIBRARY_PATH"));
            }
          if (l->GetArch32 ())
            {
              l->AddPath ("/lib/:/usr/lib/:/lib/i386-linux-gnu/:/usr/lib/i386-linux-gnu/");
            }
          if (l->GetArch64 ())
            {
              l->AddPath ("/lib64/:/usr/lib64/:/lib/x86_64-linux-gnu/:/usr/lib/x86_64-linux-gnu/:/lib/:/usr/lib/");
            }
          if (l->SearchLibs ())
            {
              done.insert (name.second);
              mapScanned [name.second] = l;
              set <pair<string,string> > full = l->GetLibs ();

              for (set <pair<string,string> >::const_iterator ii = full.begin ();
                   ii != full.end (); ++ii)
                {
                  string lalib = (*ii).second;

                  if ((todo.find (*ii) == todo.end ())
                      &&
                      (done.end () == done.find (lalib)))
                    {
                      todo.insert (*ii);
                    }
                }
            }
        }
    }

  if (0 == mapScanned[f])
    {
      NS_LOG_DEBUG ("No libraries found for " << f);
      return;
    }

  if (!mapScanned[f]->CalcDepth (mapScanned.size (), mapScanned))
    {
      NS_LOG_ERROR ("There is a loop in dependencies of " << f);
      return;
    }
  map <int, vector <SharedLibrary*> > tri;
  int maxi = 0;
  for (map <string, SharedLibrary*>::const_iterator i = mapScanned.begin ();
       i != mapScanned.end (); i++)
    {
      int p = i->second->GetDepth ();

      if (p > maxi)
        {
          maxi = p;
        }
      if (tri.find (p) == tri.end ())
        {
          vector <SharedLibrary*> vide;
          tri [p] = vide;
        }
      tri [p].push_back (i->second);
    }
  for (int j = 0 ; j <= maxi ; j++)
    {
      vector <SharedLibrary*> vectore = tri [j];
      for (vector<SharedLibrary*>::const_iterator s = vectore.begin (); s != vectore.end (); s++)
        {
          struct ElfDependencies::Dependency d;
          d.required = (*s)->GetName (0);
          d.found = (*s)->GetName (1);
          m_deps.push_back (d);
        }
    }
  for (vector<SharedLibrary*>::const_iterator s = toFree.begin (); s != toFree.end (); s++)
    {
      delete (*s);
    }
}

ElfLdd::ElfLdd (string s, string f)
{
  Loop (s, f);
}

std::vector<struct ElfDependencies::Dependency>
ElfLdd::GetDeps () const
{
  return m_deps;
}

ElfDependencies::Iterator
ElfLdd::Begin (void) const
{
  return m_deps.begin ();
}
ElfDependencies::Iterator
ElfLdd::End (void) const
{
  return m_deps.end ();
}

} // namespace ns3


