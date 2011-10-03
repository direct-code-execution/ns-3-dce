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
#ifndef FILE_USAGE_H
#define FILE_USAGE_H

namespace ns3 {

class UnixFd;

/**
* \brief A FileUsage is used to count the usage of fd by a same process in order to prevent un wanted delete
*
*/

class FileUsage
{
public:
  FileUsage (int fd, UnixFd *file);
  ~FileUsage ();

  // a new thread is using the corresponding UnixFd.
  void IncUsage ();
  // Remove one and return true if count == 0 and closing = true
  bool DecUsage ();

  // Return true if m_count == 0 else position closing to true.
  bool CanForget ();

  // Inc count and return file reference
  UnixFd* GetFileInc ();

  // Return the file
  UnixFd* GetFile () const;

  // Return true the close was already called for the corresponding fd.
  bool IsClosed () const;

  // When a process finish we can forget all usage count
  void NullifyUsage ();

private:
  int m_count;
  int const m_fd;
  UnixFd* const m_file;
  bool m_closing;
};

}

#endif // FILE_USAGE_H
