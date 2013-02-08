#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <iostream>

#define SERVER_PORT 2000

static void * client_run (void *ctx)
{
  sleep (1);
  int sock;
  sock = socket (PF_INET, SOCK_STREAM, 0);

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons (SERVER_PORT);

  struct hostent *host = gethostbyname ("127.0.0.1");
  memcpy (&addr.sin_addr.s_addr, host->h_addr_list[0], host->h_length);

  int result;
  result = connect (sock, (const struct sockaddr *) &addr, sizeof (addr));

  uint8_t buf[1024];

  memset (buf, 0x66, 20);
  memset (buf + 20, 0x67, 1004);

  for (uint32_t i = 0; i < 100000; i++)
    {
      ssize_t n = 1024;
      while (n > 0)
        {
          ssize_t written = write (sock, buf, 1024);
          std::cout << "write " << written << "bytes" << std::endl;
          n -= written;
        }
      sleep (1);
    }

  std::cout << "did write all buffers" << std::endl;

  close (sock);

  return 0;
}

static void * server_run (void *ctx)
{
  int sock;
  sock = socket (PF_INET, SOCK_STREAM, 0);

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons (SERVER_PORT);
  addr.sin_addr.s_addr = INADDR_ANY;

  int status;
  status = bind (sock, (const struct sockaddr *) &addr, sizeof (addr));
  status = listen (sock, 1);


  int fd = accept (sock, 0, 0);

  uint8_t buf[10240];

  memset (buf, 0, 10240);

  for (uint32_t i = 0; i < 10000; i++)
    {
      ssize_t n = 10240;
      while (n > 0)
        {
          ssize_t bytes_read = read (fd, buf, 10240);
          std::cout << "read " << bytes_read << "bytes" << std::endl;
          n -= bytes_read;
        }
      sleep (1);
    }

  std::cout << "did read all buffers" << std::endl;

  close (sock);
  close (fd);

  return 0;
}


int main (int argc, char *argv[])
{
  pthread_t client, server;
  pthread_create (&client, 0, &client_run, 0);
  pthread_create (&server, 0, &server_run, 0);
  pthread_join (client, 0);
  pthread_join (server, 0);

  return 0;
}
