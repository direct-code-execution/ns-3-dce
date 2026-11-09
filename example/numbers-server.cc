/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
//
// A tiny HTTP server built as a DCE application, for the dce-browser
// example: GET /next answers the next number of a series as JSON, GET /
// a page whose JavaScript polls /next and shows the results. Usage:
// numbers-server [port] (default 8080). Single threaded, one request per
// connection.
//
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <time.h>

static const char *PAGE =
  "<html><head><title>Live number series</title>\n"
  "<style>body{font-family:sans-serif;margin:2em;background:#f4f6fa}h1{color:#1f4e79}"
  "#log{font-family:monospace;background:#fff;border:2px solid #1f4e79;padding:1em;height:14em;overflow:auto}"
  ".ok{color:#1a7f37}.err{color:#b00}</style></head>\n"
  "<body><h1>Live number series</h1>\n"
  "<p>This page's JavaScript asks the server for the next Fibonacci number twice a second\n"
  "(<code>fetch('/next')</code>). Every request is an HTTP exchange over the simulated\n"
  "802.11n link between the two stations, served by a tiny HTTP server running inside\n"
  "the simulation.</p>\n"
  "<p>Requests: <b id=count>0</b>, failures: <b id=fail>0</b>, last round trip: <b id=rtt>-</b> ms</p>\n"
  "<div id=log></div>\n"
  "<p><a href=\"http://10.1.1.1/\">Back to the site</a></p>\n"
  "<script>\n"
  "var count = 0, fail = 0;\n"
  "function tick() {\n"
  "  var t0 = Date.now();\n"
  "  fetch('/next').then(function (r) { return r.json(); }).then(function (j) {\n"
  "    count++;\n"
  "    document.getElementById('count').textContent = count;\n"
  "    document.getElementById('rtt').textContent = Date.now() - t0;\n"
  "    var log = document.getElementById('log');\n"
  "    var line = document.createElement('div');\n"
  "    line.className = 'ok';\n"
  "    line.textContent = '#' + j.n + '  fib = ' + j.value + '  (server time ' + j.time + ' s)';\n"
  "    log.appendChild(line);\n"
  "    log.scrollTop = log.scrollHeight;\n"
  "  }).catch(function (e) {\n"
  "    fail++;\n"
  "    document.getElementById('fail').textContent = fail;\n"
  "  });\n"
  "}\n"
  "setInterval(tick, 500);\n"
  "tick();\n"
  "</script></body></html>\n";

static void
Respond (int fd, const char *status, const char *type, const std::string &body)
{
  char header[512];
  int n = snprintf (header, sizeof (header),
                    "HTTP/1.1 %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\n"
                    "Access-Control-Allow-Origin: *\r\nCache-Control: no-store\r\nConnection: close\r\n\r\n",
                    status, type, body.size ());
  if (write (fd, header, n) < 0 || write (fd, body.data (), body.size ()) < 0)
    {
      perror ("numbers-server: write");
    }
}

int
main (int argc, char *argv[])
{
  int port = argc > 1 ? atoi (argv[1]) : 8080;
  int server = socket (AF_INET, SOCK_STREAM, 0);
  if (server < 0)
    {
      perror ("numbers-server: socket");
      return 1;
    }
  int one = 1;
  setsockopt (server, SOL_SOCKET, SO_REUSEADDR, &one, sizeof (one));
  struct sockaddr_in addr;
  memset (&addr, 0, sizeof (addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl (INADDR_ANY);
  addr.sin_port = htons (port);
  if (bind (server, (struct sockaddr *)&addr, sizeof (addr)) < 0 || listen (server, 16) < 0)
    {
      perror ("numbers-server: bind/listen");
      return 1;
    }
  printf ("numbers-server: listening on port %d\n", port);
  fflush (stdout);

  unsigned long long a = 0, b = 1; // Fibonacci state
  unsigned long n = 0;
  while (true)
    {
      struct sockaddr_in peer;
      socklen_t len = sizeof (peer);
      int fd = accept (server, (struct sockaddr *)&peer, &len);
      if (fd < 0)
        {
          perror ("numbers-server: accept");
          continue;
        }
      char req[2048];
      ssize_t got = read (fd, req, sizeof (req) - 1);
      if (got <= 0)
        {
          close (fd);
          continue;
        }
      req[got] = 0;
      char method[8], path[256];
      if (sscanf (req, "%7s %255s", method, path) != 2)
        {
          Respond (fd, "400 Bad Request", "text/plain", "bad request\n");
        }
      else if (strcmp (path, "/next") == 0)
        {
          struct timespec now;
          clock_gettime (CLOCK_REALTIME, &now);
          char body[160];
          snprintf (body, sizeof (body), "{\"n\": %lu, \"value\": %llu, \"time\": %ld.%03ld}\n",
                    n, a, (long)now.tv_sec, now.tv_nsec / 1000000);
          Respond (fd, "200 OK", "application/json", body);
          unsigned long long next = a + b;
          a = b;
          b = next;
          n++;
          if (n % 50 == 0)
            {
              printf ("numbers-server: %lu requests served from %s\n", n, inet_ntoa (peer.sin_addr));
              fflush (stdout);
            }
        }
      else if (strcmp (path, "/") == 0 || strcmp (path, "/index.html") == 0)
        {
          Respond (fd, "200 OK", "text/html; charset=utf-8", PAGE);
        }
      else
        {
          Respond (fd, "404 Not Found", "text/plain", "not found\n");
        }
      close (fd);
    }
  return 0;
}
