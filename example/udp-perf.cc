#include <sys/timerfd.h>
#include <time.h>
#include <sys/time.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdint.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <poll.h>

extern long long int llrintl (long double x);
extern long int lrintl (long double x);
static uint64_t current_time (void);

#define CHECK_INT_ARG(arg, name, value)                           \
  if (strncmp (arg, "--" name "=", strlen ("--" name "=")) == 0)      \
    {                                                             \
      value = atoi (arg + strlen ("--" name "="));                  \
    }
#define CHECK_LLINT_ARG(arg, name, value)                                 \
  if (strncmp (arg, "--" name "=", strlen ("--" name "=")) == 0)      \
    {                                                             \
      value = atoll (arg + strlen ("--" name "="));                 \
    }
#define CHECK_STR_ARG(arg, name, value)                           \
  if (strncmp (arg, "--" name "=", strlen ("--" name "=")) == 0)      \
    {                                                             \
      value = arg + strlen ("--" name "=");                         \
    }

#if 0
static void set_txbuf_size (int fd, int buffer_size)
{
  int msg_size = buffer_size;
  int status = setsockopt (fd, SOL_SOCKET, SO_SNDBUF, (char*)&msg_size, sizeof (msg_size));
  if (status == -1)
    {
      printf ("Unable to set socket buffer size to %d\n", msg_size);
      exit (1);
    }
}
#endif

static void run_client (int to_port, const char *to_ip, int pkt_size,
                        long long bytes_per_second, int dur, int nodes, long long bw)
{
  int fd = socket (AF_INET, SOCK_DGRAM, 0);
  if (fd == -1)
    {
      printf ("Unable to create udp socket\n");
      exit (1);
    }
  int status;
  status = fcntl (fd, F_GETFL, 0);
  if (status == -1)
    {
      perror ("fcntl");
      exit (1);
    }
  status = fcntl (fd, F_SETFL, status | O_NONBLOCK);
  if (status == -1)
    {
      perror ("fcntl");
      exit (1);
    }
  status = fcntl (fd, F_GETFL, 0);
#if 0
  set_txbuf_size (fd, 1 << 20);
#endif

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = 0;
  addr.sin_addr.s_addr = htonl (INADDR_ANY);
  status = bind (fd, (struct sockaddr*)&addr, sizeof (addr));
  if (status == -1)
    {
      printf ("Unable to bind\n");
      exit (1);
    }

  char *buffer = (char*)malloc (pkt_size);
  memset (buffer, 0, pkt_size);

  int64_t pps = llrintl (1.0L * bytes_per_second / pkt_size);
  int64_t ns_delay = llrintl (1000000000.0L / pps);
  if (ns_delay <= 100000)
    {
      ns_delay = 100000;
    }

  // 10^9ns=1s
  double packets_per_delay = 1.0L * bytes_per_second * ns_delay /
    pkt_size / 1000000000.0L;
  printf ("bw = %lld, psize = %d, pps = %lld, ns_delay = %lld, ppd = %f\n",
          (signed long long) bytes_per_second, pkt_size, (signed long long) pps,
          (signed long long) ns_delay, packets_per_delay);

  int timerfd = timerfd_create (CLOCK_MONOTONIC, 0);
  struct itimerspec timer;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_nsec = ns_delay;
  timer.it_value.tv_sec = 0;
  timer.it_value.tv_nsec = ns_delay;
  timerfd_settime (timerfd, 0, &timer, 0);

  struct sockaddr_in to;
  to.sin_family = AF_INET;
  to.sin_port = htons (to_port);
  to.sin_addr.s_addr = inet_addr (to_ip);

  int64_t sent = 0, total_misses = 0, snd_skipped = 0;
  int expires = 0;
  uint64_t last_print_time = current_time ();
  int cpt = 0;
  long total = 0;
  while (true)
    {
      uint64_t current = current_time ();
      if (current > last_print_time + 1000000)
        {
          last_print_time = current;
          //printf("Timer misses: %ld, skipped sends: %ld\n", total_misses, snd_skipped);
          if (dur > 0)
            {
              cpt++;
              if (cpt >= dur)
                {
                  printf ("duration %d, total write %ld total packets %ld\n", dur, total, sent);
                  FILE *f = fopen ("client.txt","a");
                  fprintf (f, "%lld %d %ld\n",bw, nodes, sent);
                  fclose (f);
                  exit (0);
                }
            }
        }
      uint64_t skipped;
      ssize_t bytes_read;
      bytes_read = read (timerfd, &skipped, 8);
      //printf("skipped: %ld\n", skipped);
      expires += skipped;
      total_misses += skipped - 1;
      long i, tosend = lrintl (expires * packets_per_delay) - sent;
      for (i = 0; i < tosend; i++)
        {
          ssize_t written = sendto (fd, buffer, pkt_size, MSG_DONTWAIT,
                                    (struct sockaddr *)&to, sizeof (to));
          if (written == -1 && (errno == EWOULDBLOCK || errno == EAGAIN))
            {
              snd_skipped += tosend - i;
              break;
            }
          if (written == -1)
            {
              perror ("sentto");
              exit (1);
            }
          total += written;
          sent++;
        }
    }

  free (buffer);

  status = close (fd);
  if (status == -1)
    {
      printf ("Unable to close file descriptor %d\n", fd);
      exit (1);
    }

}

static uint64_t current_time (void)
{
  struct timeval tv;
  int status = gettimeofday (&tv, 0);
  if (status == -1)
    {
      printf ("Unable to get current time\n");
      exit (1);
    }
  uint64_t current_time = tv.tv_sec;
  current_time *= 1000000;
  current_time += tv.tv_usec;
  return current_time;
}

static void run_server (int port, int dur, int nodes, long long bytes_per_second)
{
  int fd = socket (AF_INET, SOCK_DGRAM, 0);
  if (fd == -1)
    {
      printf ("Unable to create udp socket\n");
      exit (1);
    }
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons (port);
  addr.sin_addr.s_addr = htonl (INADDR_ANY);
  int status = bind (fd, (struct sockaddr*)&addr, sizeof (addr));
  if (status == -1)
    {
      printf ("Unable to bind to port %d\n", port);
      exit (1);
    }

  uint64_t full_total = 0;
  uint64_t last_print_time = current_time ();
  int64_t total_received = 0, total_reads = 0;
  int64_t buffer_size = 1 << 17;
  uint64_t total_packet = 0;
  int cpt = 0;

  char *buffer = (char*)malloc (buffer_size);
  while (true)
    {
      uint64_t current = current_time ();
      if (current > last_print_time + 1000000)
        {
          last_print_time = current;
          full_total += total_received;
          int64_t q = total_reads;
          if (q == 0)
            {
              q = 1;
            }
          printf ("received=%lld bytes, %lld reads (@%lld bytes) %llu\n",
                  (signed long long) total_received,
                  (signed long long) total_reads,
                  (signed long long)(total_received / q),
                  (unsigned long long) full_total);

          total_received = total_reads = 0;
          if (dur > 0)
            {
              cpt++;
              if (cpt > dur)
                {
                  printf ("duration %d, total read %ld total packet %ld\n", dur, full_total, total_packet);
                  FILE *f = fopen ("server.txt","a");
                  fprintf (f, "%lld %d %ld\n",bytes_per_second, nodes, total_packet);
                  fclose (f);
                  exit (0);
                }
            }
        }
      int r = 1;
      if (dur > 0)
        {
          struct pollfd p =
          {
            fd, POLLIN, 0
          };
          int t = dur - cpt;
          if (t <= 0)
            {
              t = 1;
            }
          r = poll (&p, 1, t * 1000);
        }
      if (1 == r)
        {
          ssize_t received = recvfrom (fd, buffer, buffer_size, 0, 0, 0);
          if (received > 0)
            {
              total_reads++;
              total_packet++;
              total_received += received;
            }
        }
    }
  free (buffer);

  status = close (fd);
  if (status == -1)
    {
      printf ("Unable to close file descriptor %d\n", fd);
      exit (1);
    }
}

int main (int argc, char *argv[])
{
  long long bytes_per_second = 1000000;
  int pkt_size = 1500;
  int port = 5000;
  int nodes = 0;
  int duration = 0;
  const char *to_ip = "127.0.0.1";
  bool client = false;
  char **arg = argv;
  while (*arg != 0)
    {
      CHECK_INT_ARG (*arg, "pktsize", pkt_size);
      CHECK_LLINT_ARG (*arg, "bandwidth", bytes_per_second);
      CHECK_INT_ARG (*arg, "port", port);
      CHECK_INT_ARG (*arg, "nodes", nodes);
      CHECK_INT_ARG (*arg, "duration", duration);
      CHECK_STR_ARG (*arg, "host", to_ip);
      if (strcmp (*arg, "--client") == 0)
        {
          client = true;
        }
      arg++;
    }
  setlinebuf (stdout);
  if (client)
    {
      run_client (port, to_ip, pkt_size, bytes_per_second / 8.5, duration, nodes, bytes_per_second);
    }
  else
    {
      run_server (port, duration, nodes, bytes_per_second);
    }

  return 0;
}
