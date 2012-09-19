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
#ifndef ELF_LDD_H_
#define ELF_LDD_H_

#include "elf-dependencies.h"
#include <string>
#include <vector>

namespace ns3 {

class SharedLibrary;

class ElfLdd
{
public:
  ElfLdd (std::string shortName, std::string fullName);

  std::vector<struct ElfDependencies::Dependency> GetDeps () const;

  ElfDependencies::Iterator Begin (void) const;
  ElfDependencies::Iterator End (void) const;


private:
  void Loop (std::string s, std::string f);
  SharedLibrary* ExtractLibraries (std::string s, std::string f);
  std::vector<struct ElfDependencies::Dependency> m_deps;
};

} // namespace ns3

#endif /* ELF_LDD_H_ */
