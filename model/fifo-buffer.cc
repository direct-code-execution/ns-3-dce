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
#include "fifo-buffer.h"
#include "ns3/log.h"
#include <stdlib.h>
#include <string.h>

NS_LOG_COMPONENT_DEFINE ("FifoBuffer");

#define MIN_ALLOC 1024

namespace ns3 {
FifoBuffer::FifoBuffer (size_t mxSz) : m_maxSize (mxSz),
                                       m_buffer (0),
                                       m_read (0)
                                       ,
                                       m_fill (0),
                                       m_size (0)
{
}
FifoBuffer::~FifoBuffer (void)
{
  if (m_buffer)
    {
      free (m_buffer);
      m_buffer = 0;
    }
}
ssize_t
FifoBuffer::Write (uint8_t *buf, size_t len)
{
  NS_LOG_FUNCTION ("r:" << m_read << " f:" << m_fill << " s:" <<  m_size);
  size_t capa = m_size - m_fill;

  if (len <= capa)
    {
      memcpy (m_buffer + m_fill, buf, len);
      m_fill += len;
      return len;
    }
  if ((m_read > 0) && (m_size <= m_maxSize))
    {
      if (m_fill > m_read)
        {
          memmove (m_buffer, m_buffer + m_read, m_fill - m_read);
        }
      m_fill -= m_read;
      m_read = 0;
      return Write (buf, len);
    }
  if (m_size == m_maxSize)
    {
      if (capa > 0)
        {
          memcpy (m_buffer + m_fill, buf, capa);
          m_fill += capa;
          return capa;
        }
    }
  else
    {
      size_t newSize = len + m_fill;

      if (newSize - m_size < MIN_ALLOC)
        {
          newSize = m_size + MIN_ALLOC;
        }
      if (newSize > m_maxSize)
        {
          newSize = m_maxSize;
        }

      uint8_t *newBuf = (uint8_t*) malloc (newSize);
      if (!newBuf)
        {
          return -1;
        }
      memcpy (newBuf, m_buffer + m_read, m_fill - m_read);
      free (m_buffer);
      m_buffer = newBuf;
      m_fill -= m_read;
      m_read = 0;
      m_size = newSize;

      return Write (buf, len);
    }
  return 0;
}
ssize_t
FifoBuffer::Read (uint8_t *buf, size_t len)
{
  NS_LOG_FUNCTION ("r:" << m_read << " f:" << m_fill << " s:" <<  m_size);
  size_t capa = m_fill - m_read;

  if (capa > 0)
    {
      size_t l = std::min (len, capa);

      memcpy (buf, m_buffer + m_read, l);
      m_read += l;

      return l;
    }
  return 0;
}
ssize_t
FifoBuffer::GetSize ()
{
  return m_fill - m_read;
}
ssize_t
FifoBuffer::GetSpace ()
{
  return m_maxSize - GetSize ();
}
} // namespace ns3
