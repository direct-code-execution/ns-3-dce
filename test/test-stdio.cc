#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>

#include "test-macros.h"

static void test_fopen (void)
{
  int status;
  status = unlink ("X");

  FILE *file;
  file = fopen ("X", "f");
  TEST_ASSERT_EQUAL (file, 0);
  TEST_ASSERT_EQUAL (errno, EINVAL);
  file = fopen ("X", "r");
  TEST_ASSERT_EQUAL (file, 0);
  file = fopen ("X", "r+");
  TEST_ASSERT_EQUAL (file, 0);

  file = fopen ("X", "w");
  TEST_ASSERT_UNEQUAL (file, 0);
  status = fclose (file);
  TEST_ASSERT_EQUAL (status, 0);

  file = fopen ("X", "r");
  TEST_ASSERT_UNEQUAL (file, 0);
  status = fclose (file);
  TEST_ASSERT_EQUAL (status, 0);

  file = fopen ("X", "r+");
  TEST_ASSERT_UNEQUAL (file, 0);
  status = fclose (file);
  TEST_ASSERT_EQUAL (status, 0);

  file = fopen ("X", "w");
  TEST_ASSERT_UNEQUAL (file, 0);
  status = fclose (file);
  TEST_ASSERT_EQUAL (status, 0);

  status = unlink ("X");
  TEST_ASSERT_EQUAL (status, 0);

  file = fopen ("X", "w+");
  TEST_ASSERT_UNEQUAL (file, 0);
  status = fclose (file);
  TEST_ASSERT_EQUAL (status, 0);

  file = fopen ("X", "w+");
  TEST_ASSERT_UNEQUAL (file, 0);
  status = fclose (file);
  TEST_ASSERT_EQUAL (status, 0);

  status = unlink ("X");
  TEST_ASSERT_EQUAL (status, 0);
}

static void test_freadwrite (void)
{
  unsigned char buffer[] = {
    0xff, 0xfe, 0xfd, 0x00,
    0x02, 0x05, 0x12, 0x15
  };
  int status;
  status = unlink ("X");

  FILE *file;
  file = fopen ("X", "w");
  TEST_ASSERT_UNEQUAL (file, 0);

  size_t items;
  items = fwrite (buffer, sizeof(buffer), 1, file);
  TEST_ASSERT_EQUAL (items, 1);
  items = fwrite (buffer, 1, sizeof(buffer), file);
  TEST_ASSERT_EQUAL (items, sizeof(buffer));

  items = fread (buffer, 1, 1, file);
  TEST_ASSERT_EQUAL (items, 0);
  TEST_ASSERT_UNEQUAL (ferror (file), 0);
  clearerr (file);
  TEST_ASSERT_EQUAL (ferror (file), 0);

  status = fclose (file);
  TEST_ASSERT_EQUAL (status, 0);

  // Now, try to open a new file
  file = fopen ("X", "r");
  TEST_ASSERT_UNEQUAL (file, 0);

  char read_buffer[sizeof(buffer)];
  items = fread (read_buffer, 1, sizeof(read_buffer), file);
  TEST_ASSERT_EQUAL (items, sizeof(read_buffer));
  items = fread (read_buffer, sizeof(read_buffer), 1, file);
  TEST_ASSERT_EQUAL (items, 1);

  items = fread (read_buffer, sizeof(read_buffer), 1, file);
  TEST_ASSERT_EQUAL (items, 0);
  TEST_ASSERT_UNEQUAL (feof (file), 0);
  TEST_ASSERT_EQUAL (ferror (file), 0);
  clearerr (file);
  TEST_ASSERT_EQUAL (feof (file), 0);
  TEST_ASSERT_EQUAL (ferror (file), 0);

  status = fclose (file);
  TEST_ASSERT_EQUAL (status, 0);
}

static void
test_freopen (void)
{
  int status;
  // cleanup
  unlink ("X");

  char c = 0x66;
  FILE *wo = fopen ("X", "w");
  TEST_ASSERT_UNEQUAL (wo, 0);
  status = fwrite (&c, 1, 1, wo);
  TEST_ASSERT_EQUAL (status, 1);
  status = fread (&c, 1, 1, wo);
  TEST_ASSERT_EQUAL (status, 0);
  TEST_ASSERT_EQUAL (feof (wo), 0);
  TEST_ASSERT_UNEQUAL (ferror (wo), 0);

  FILE *ro = freopen ("X", "r", wo);
  TEST_ASSERT_EQUAL (wo, ro);
  status = fread (&c, 1, 1, ro);
  TEST_ASSERT_EQUAL (status, 1);

  fclose (ro);
}

void test_fcloseall (void)
{
  int status = fcloseall ();
  TEST_ASSERT_EQUAL (status, 0);
}

void test_fileno (void)
{
  int fd = fileno (stdout);
  TEST_ASSERT_EQUAL (fd, 1);
  fd = fileno (stderr);
  TEST_ASSERT_EQUAL (fd, 2);
  fd = fileno (stdin);
  TEST_ASSERT_EQUAL (fd, 0);
}
void test_seek (void)
{
  // cleanup
  unlink ("X");
  // let's write a file
  unsigned char buffer[] = {
    0xff, 0xfe, 0xfd, 0x00,
    0x02, 0x05, 0x12, 0x15
  };
  FILE *file;
  file = fopen ("X", "w+");
  TEST_ASSERT_UNEQUAL (file, 0);

  size_t items;
  items = fwrite (buffer, sizeof(buffer), 1, file);
  TEST_ASSERT_EQUAL (items, 1);
  items = fwrite (buffer, 1, sizeof(buffer), file);
  TEST_ASSERT_EQUAL (items, sizeof(buffer));
  int status = fflush (file);
  TEST_ASSERT_EQUAL (status, 0);
  status = fclose (file);
  TEST_ASSERT_EQUAL (status, 0);

  // now, let's read data.
  FILE *ro = fopen ("X", "r");
  TEST_ASSERT_UNEQUAL (ro, 0);
  long current = ftell (ro);
  TEST_ASSERT_EQUAL (current, 0);
  status = fseek (ro, 0, SEEK_SET);
  TEST_ASSERT_EQUAL (status, 0);
  status = fseek (ro, 0, SEEK_END);
  TEST_ASSERT_EQUAL (status, 0);
  current = ftell (ro);
  TEST_ASSERT_EQUAL (current, sizeof(buffer) * 2);
  status = fseek (ro, -5, SEEK_END);
  TEST_ASSERT_EQUAL (status, 0);
  current = ftell (ro);
  TEST_ASSERT_EQUAL (current, sizeof(buffer) * 2 - 5);
  status = fseek (ro, +3, SEEK_CUR);
  TEST_ASSERT_EQUAL (status, 0);
  current = ftell (ro);
  TEST_ASSERT_EQUAL (current, sizeof(buffer) * 2 - 5 + 3);
  rewind (ro);
  current = ftell (ro);
  TEST_ASSERT_EQUAL (current, 0);

  fpos_t pos;
  status = fgetpos (ro, &pos);
  TEST_ASSERT_EQUAL (status, 0);
  status = fseek (ro, +3, SEEK_CUR);
  TEST_ASSERT_EQUAL (status, 0);
  current = ftell (ro);
  TEST_ASSERT_EQUAL (current, 3);
  status = fsetpos (ro, &pos);
  TEST_ASSERT_EQUAL (status, 0);
  current = ftell (ro);
  TEST_ASSERT_EQUAL (current, 0);


  status = fclose (ro);
  TEST_ASSERT_EQUAL (status, 0);
}
void test_buf (void)
{
  // cleanup
  unlink ("X");

  FILE *f = fopen ("X", "w");
  char buffer[BUFSIZ];
  // enable buffered output
  setbuf (f, buffer);
  // disable buffering.
  setbuf (f, 0);

  // enable buffered output
  setbuffer (f, buffer, BUFSIZ);
  // disable buffering.
  setbuffer (f, 0, 0);

  // enable line buffering
  setlinebuf (f);

  int status = setvbuf (f, buffer, _IOFBF, BUFSIZ);
  TEST_ASSERT_EQUAL (status, 0);
  status = setvbuf (f, 0, _IONBF, 0);
  TEST_ASSERT_EQUAL (status, 0);
  status = setvbuf (f, buffer, _IOLBF, BUFSIZ);
  TEST_ASSERT_EQUAL (status, 0);

  fclose (f);
}

void test_formatted_io (void)
{
  // cleanup
  unlink ("X");

  FILE *f = fopen ("X", "w+");

  int status = fputs ("test\n", f);
  TEST_ASSERT_UNEQUAL (status, EOF);
  long pos = ftell (f);
  TEST_ASSERT_EQUAL (pos, 5);
  status = fputc ('a', f);
  TEST_ASSERT_UNEQUAL (status, EOF);
  pos = ftell (f);
  TEST_ASSERT_EQUAL (pos, 6);
  status = putc ('a', f);
  TEST_ASSERT_UNEQUAL (status, EOF);
  pos = ftell (f);
  TEST_ASSERT_EQUAL (pos, 7);

  status = fseek (f, -2, SEEK_END);
  TEST_ASSERT_EQUAL (status, 0);
  pos = ftell (f);
  TEST_ASSERT_EQUAL (pos, 5);

  status = fgetc (f);
  TEST_ASSERT_EQUAL (status, 'a');
  status = ungetc ('b', f);
  TEST_ASSERT_EQUAL (status, 'b');
  status = fgetc (f);
  TEST_ASSERT_EQUAL (status, 'b');
  status = getc (f);
  TEST_ASSERT_EQUAL (status, 'a');

  status = fseek (f, 0, SEEK_SET);
  TEST_ASSERT_EQUAL (status, 0);

  char buffer[100];

  char *retval = fgets (buffer, 100, f);
  TEST_ASSERT_UNEQUAL (retval, 0);
  TEST_ASSERT (strcmp ("test\n", retval) == 0);


  fclose (f);

  status = putchar ('a');
  TEST_ASSERT_UNEQUAL (status, EOF);
  status = puts ("test");
  TEST_ASSERT_UNEQUAL (status, EOF);
  // getchar
  // gets
}

void test_stdin (void)
{
  int l = 0;
  char buf;
  FILE *copyIt = fopen ("/tmp/copy_stdin", "w");

  while (!feof (stdin))
    {
      int a = fread (&buf, sizeof(buf), 1, stdin);

      if (a > 0)
        {
          fwrite (&buf, sizeof(buf), 1, copyIt);
          l += a;
        }
    }

  fclose (copyIt);

  TEST_ASSERT (l > 0);

}

void test_dup (void)
{
  int fd = 0;
  int first = 0;
  int last = 0;
  fd = dup (1);

  while (fd > 0)
    {
      last = fd;
      if (!first)
        {
          first = fd;
        }

      char line[1024];

      int l = sprintf (line, "%d\n", fd);

      write (fd, line, l);

      fd = dup (fd);
    }
  fd = last;
  while (fd >= first)
    {
      close (fd);
      fd--;
    }

}

void simple_dup (void)
{
  int fd = open ("/tmp/hello",  O_RDWR | O_CREAT, 0644);
  char line[1024];

  int l = sprintf (line, "Hello Fd %d\n", fd);
  write (fd, line, l);

  int fd2 = dup (fd);

  l = sprintf (line, "Hello fd %d\n", fd2);
  write (fd2, line, l);

  close (fd);
  close (fd2);

}
extern char *__progname;

static void test_tmpfile(void)
{
  FILE *f = tmpfile();
  TEST_ASSERT(f!=NULL);
  fclose( f );
}



int main (int argc, char *argv[])
{
  printf ("ProgName: %s %s %s \n", __progname, program_invocation_name, program_invocation_short_name);
  test_fopen ();
  test_freadwrite ();
  test_freopen ();
  test_fileno ();
  test_seek ();
  test_buf ();
  test_formatted_io ();
  test_stdin ();
  test_dup ();
  simple_dup ();
  test_tmpfile();
  // Should be last because it closes all open streams, including stdout et al.
  test_fcloseall ();
  return 0;
}
