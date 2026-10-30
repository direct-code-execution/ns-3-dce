/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2026 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
 */

/*
 * What the kernel delivers on an interface, as seen by a packet socket
 * (what tcpdump sees): lkl-sniff <interface> <seconds> [<pcap file>]
 *
 * On an Ethernet-like interface (a wifi STA, mesh point...), counts the
 * frames by packet type (host, broadcast, multicast, otherhost, outgoing),
 * source, destination and protocol, after putting the interface in
 * promiscuous mode. On a monitor interface, writes the radiotap frames to
 * the pcap file. Prints the counts when done.
 */

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <map>
#include <string>
#include <tuple>

static double
Now (void)
{
  struct timespec ts;
  clock_gettime (CLOCK_MONOTONIC, &ts);
  return ts.tv_sec + ts.tv_nsec / 1e9;
}

static std::string
Mac (const uint8_t *m)
{
  char s[18];
  snprintf (s, sizeof (s), "%02x:%02x:%02x:%02x:%02x:%02x", m[0], m[1], m[2], m[3], m[4], m[5]);
  return s;
}

static const char *
TypeName (int type)
{
  switch (type)
    {
    case PACKET_HOST: return "host";
    case PACKET_BROADCAST: return "broadcast";
    case PACKET_MULTICAST: return "multicast";
    case PACKET_OTHERHOST: return "otherhost";
    case PACKET_OUTGOING: return "outgoing";
    default: return "other";
    }
}

int
main (int argc, char *argv[])
{
  if (argc < 3)
    {
      fprintf (stderr, "usage: %s <interface> <seconds> [<pcap file>]\n", argv[0]);
      return 2;
    }
  const char *ifname = argv[1];
  double duration = atof (argv[2]);
  int ifindex = if_nametoindex (ifname);
  if (ifindex == 0)
    {
      perror ("if_nametoindex");
      return 1;
    }
  int fd = socket (AF_PACKET, SOCK_RAW, htons (ETH_P_ALL));
  if (fd < 0)
    {
      perror ("socket");
      return 1;
    }
  struct sockaddr_ll sll;
  memset (&sll, 0, sizeof (sll));
  sll.sll_family = AF_PACKET;
  sll.sll_protocol = htons (ETH_P_ALL);
  sll.sll_ifindex = ifindex;
  if (bind (fd, (struct sockaddr *)&sll, sizeof (sll)) < 0)
    {
      perror ("bind");
      return 1;
    }
  FILE *pcap = 0;
  if (argc > 3)
    {
      pcap = fopen (argv[3], "w");
      // Global header: radiotap (127), the link type of monitor interfaces.
      uint32_t header[6] = {0xa1b2c3d4, 0x00040002, 0, 0, 65535, 127};
      fwrite (header, sizeof (header), 1, pcap);
    }
  else
    {
      struct packet_mreq mr;
      memset (&mr, 0, sizeof (mr));
      mr.mr_ifindex = ifindex;
      mr.mr_type = PACKET_MR_PROMISC;
      if (setsockopt (fd, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof (mr)) < 0)
        {
          perror ("PACKET_ADD_MEMBERSHIP");
          return 1;
        }
    }

  // type, source, destination, protocol -> frames, bytes
  std::map<std::tuple<int, std::string, std::string, int>, std::pair<uint64_t, uint64_t> > counts;
  uint64_t frames = 0;
  uint8_t buf[4096];
  double end = Now () + duration;
  while (Now () < end)
    {
      struct pollfd pfd = {fd, POLLIN, 0};
      int timeout = (int)((end - Now ()) * 1000) + 1;
      if (poll (&pfd, 1, timeout) <= 0)
        {
          continue;
        }
      struct sockaddr_ll from;
      socklen_t fromlen = sizeof (from);
      ssize_t n = recvfrom (fd, buf, sizeof (buf), 0, (struct sockaddr *)&from, &fromlen);
      if (n <= 0)
        {
          continue;
        }
      frames++;
      if (pcap)
        {
          double t = Now ();
          uint32_t record[4] = {(uint32_t)t, (uint32_t)((t - (uint32_t)t) * 1e6),
                                (uint32_t)n, (uint32_t)n};
          fwrite (record, sizeof (record), 1, pcap);
          fwrite (buf, n, 1, pcap);
          continue;
        }
      if (n < 14)
        {
          continue;
        }
      std::pair<uint64_t, uint64_t> &c =
        counts[std::make_tuple (from.sll_pkttype, Mac (buf + 6), Mac (buf), (buf[12] << 8) | buf[13])];
      c.first++;
      c.second += n;
    }
  if (pcap)
    {
      fclose (pcap);
    }
  for (auto &i : counts)
    {
      printf ("sniff %s type=%s src=%s dst=%s proto=0x%04x frames=%llu bytes=%llu\n", ifname,
              TypeName (std::get<0> (i.first)), std::get<1> (i.first).c_str (),
              std::get<2> (i.first).c_str (), std::get<3> (i.first),
              (unsigned long long)i.second.first, (unsigned long long)i.second.second);
    }
  printf ("sniff %s total frames=%llu\n", ifname, (unsigned long long)frames);
  fflush (stdout);
  close (fd);
  return 0;
}
