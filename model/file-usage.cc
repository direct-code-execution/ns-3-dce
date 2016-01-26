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
#include "file-usage.h"
#include "ns3/log.h"
#include "unix-fd.h"

NS_LOG_COMPONENT_DEFINE ("DceFileUsage");

namespace ns3 {
FileUsage::FileUsage (int fd, UnixFd *file)
  : m_count (0),
    m_fd (fd),
    m_file (file),
    m_closing (0)
{
}

FileUsage::~FileUsage ()
{
  m_file->DecFdCount ();
  m_file->Unref ();
}

void
FileUsage::IncUsage ()
{
  m_count++;
}
bool
FileUsage::DecUsage ()
{
  m_count--;
  return ((0 == m_count)&&(m_closing));
}
bool
FileUsage::CanForget ()
{
  if (0 == m_count)
    {
      return true;
    }
  m_closing = true;
  return false;
}
UnixFd*
FileUsage::GetFileInc ()
{
  IncUsage ();
  return m_file;
}
UnixFd*
FileUsage::GetFile () const
{
  return m_file;
}
bool
FileUsage::IsClosed () const
{
  return m_closing;
}
void
FileUsage::NullifyUsage ()
{
  m_count = 0;
}
}
