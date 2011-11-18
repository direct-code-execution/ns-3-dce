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
#ifndef FIFO_BUFFER_H
#define FIFO_BUFFER_H

#include <stdint.h>
#include <unistd.h>

namespace ns3 {
class FifoBuffer
{
public:
  FifoBuffer (size_t mxSz);
  ~FifoBuffer (void);

  ssize_t Write (uint8_t *buf, size_t len);
  ssize_t Read (uint8_t *buf, size_t len);
  ssize_t GetSize ();
  ssize_t GetSpace ();

private:
  const size_t m_maxSize;
  uint8_t *m_buffer;
  size_t m_read;
  size_t m_fill;
  size_t m_size;
};
}

#endif // FIFO_BUFFER_H
