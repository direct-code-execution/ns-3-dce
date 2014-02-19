#define _GNU_SOURCE 1
#include "test-macros.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <limits.h>

static void test_open_exclusive (void)
{
  int status, fd;

  // first, make sure we get an empty file
  fd = open ("X", O_CREAT | O_TRUNC | O_RDWR, S_IRWXU);
  TEST_ASSERT_UNEQUAL (fd, -1);

  // create a second time exclusive. fail.
  status = open ("X", O_CREAT | O_EXCL | O_RDWR, S_IRWXU);
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, EEXIST);

  status = close (fd);
  TEST_ASSERT_EQUAL (status, 0);

  // a second time
  status = close (fd);
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, EBADF);

  // invalid fd
  status = close (-1);
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, EBADF);

  // cleanup
  status = unlink ("X");
  TEST_ASSERT_EQUAL (status, 0);
}

static void test_stat (void)
{
  int fd, status;

  // make sure file does not exist yet.
  fd = open ("X", O_RDONLY);
  TEST_ASSERT_EQUAL (fd, -1);
  TEST_ASSERT_EQUAL (errno, ENOENT);

  // make sure it really does not exist
  struct stat st;
  status = stat ("X", &st);
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, ENOENT);

  status = lstat ("X", &st);
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, ENOENT);

  // verify fails on invalid descriptor
  status = fstat (-1, &st);
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, EBADF);

  // first, make sure we get an empty file
  fd = open ("X", O_CREAT | O_TRUNC | O_RDWR, S_IRWXU);
  // we know that fd 0, 1, and 2 exist already
  TEST_ASSERT (fd >= 3);

  // verify new file has zero size
  status = fstat (fd, &st);
  TEST_ASSERT_EQUAL (status, 0);
  TEST_ASSERT_EQUAL (st.st_size, 0);

  // verify new file has zero size
  status = stat ("X", &st);
  TEST_ASSERT_EQUAL (status, 0);
  TEST_ASSERT_EQUAL (st.st_size, 0);

  // verify new file has zero size
  status = lstat ("X", &st);
  TEST_ASSERT_EQUAL (status, 0);
  TEST_ASSERT_EQUAL (st.st_size, 0);

  // delete file
  status = unlink ("X");
  TEST_ASSERT_EQUAL (status, 0);

  // close
  status = close (fd);
  TEST_ASSERT_EQUAL (status, 0);

  status = fstatat (42, "/etc/passwd", &st, 0);
  TEST_ASSERT_EQUAL (status, 0);

  status = fstatat (42, "passwd", &st, 0);
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, EBADF);

  fd = open ("/etc/passwd", O_RDONLY, 0);
  TEST_ASSERT (fd > 0);
  status = fstatat (fd, "passwd", &st, 0);
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, ENOTDIR);
  status = close (fd);
  TEST_ASSERT_EQUAL (status, 0);

  fd = open ("/etc", O_RDONLY, 0);
  TEST_ASSERT (fd > 0);
  status = fstatat (fd, "passwd", &st, 0);
  TEST_ASSERT_EQUAL (status, 0);
  status = close (fd);
  TEST_ASSERT_EQUAL (status, 0);
}

static void test_exists (void)
{
  int fd;

  // open non-existing file. fail
  fd = open ("X", O_RDONLY);
  TEST_ASSERT_EQUAL (fd, -1);
  TEST_ASSERT_EQUAL (errno, ENOENT);

  // open pathname with invalid directory.
  fd = open ("X/A", O_RDONLY);
  TEST_ASSERT_EQUAL (fd, -1);
  TEST_ASSERT_EQUAL (errno, ENOENT);

  // open non-existing directory. fail.
  fd = open ("X", O_DIRECTORY | O_RDONLY);
  TEST_ASSERT_EQUAL (fd, -1);
  TEST_ASSERT_EQUAL (errno, ENOENT);
}

static void test_file_remove (void)
{
  int status, fd;

  // create file
  fd = open ("X", O_CREAT | O_TRUNC | O_RDWR, S_IRWXU);
  TEST_ASSERT_UNEQUAL (fd, -1);

  // try to delete with remove.
  status = remove ("X");
  TEST_ASSERT_EQUAL (status, 0);

  // finally, close fd
  status = close (fd);
  TEST_ASSERT_EQUAL (status, 0);
}

static void test_create_dir (void)
{
  int status, fd;

  // check that directory does not exist.
  status = open ("X", O_DIRECTORY | O_RDONLY);
  TEST_ASSERT_EQUAL (status, -1);

  // create directory
  status = mkdir ("X", S_IRWXU);
  TEST_ASSERT_EQUAL (status, 0);

  // create a second time. fail.
  status = mkdir ("X", S_IRWXU);
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, EEXIST);

  // check that directory exists.
  fd = open ("X", O_DIRECTORY | O_RDONLY);
  TEST_ASSERT_UNEQUAL (fd, -1);

  // close
  status = close (fd);
  TEST_ASSERT_EQUAL (status, 0);

  // delete directory as file. fail.
  status = unlink ("X");
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, EISDIR);

  // delete directory.
  status = rmdir ("X");
  TEST_ASSERT_EQUAL (status, 0);
}

static void test_remove_dir (void)
{
  int status;

  // check that directory does not exist.
  status = open ("X", O_DIRECTORY | O_RDONLY);
  TEST_ASSERT_EQUAL (status, -1);

  // delete non-existing with remove
  status = remove ("X");
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, ENOENT);

  // create directory
  status = mkdir ("X", S_IRWXU);
  TEST_ASSERT_EQUAL (status, 0);

  // delete with remove
  status = remove ("X");
  TEST_ASSERT_EQUAL (status, 0);

  // delete a second time with remove. fail.
  status = remove ("X");
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, ENOENT);

  // delete a third time with rmdir. fail.
  status = rmdir ("X");
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, ENOENT);

  // create directory
  status = mkdir ("X", S_IRWXU);
  TEST_ASSERT_EQUAL (status, 0);

  // delete with rmdir
  status = rmdir ("X");
  TEST_ASSERT_EQUAL (status, 0);

  // delete a second time with rmdir. fail.
  status = rmdir ("X");
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, ENOENT);

  // delete a third time with remove. fail.
  status = remove ("X");
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, ENOENT);
}

static void test_subfile (void)
{
  int status, fd;
  // create directory
  status = mkdir ("X", S_IRWXU);
  TEST_ASSERT_EQUAL (status, 0);

  // create subfile.
  fd = open ("X/A", O_CREAT | O_TRUNC | O_RDWR, S_IRWXU);
  TEST_ASSERT_UNEQUAL (status, -1);

  // try to delete non-empty directory. fail.
  status = rmdir ("X");
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, ENOTEMPTY);

  // delete file but don't close it yet
  status = unlink ("X/A");
  TEST_ASSERT_EQUAL (status, 0);

  // delete empty directory.
  status = rmdir ("X");
  TEST_ASSERT_EQUAL (status, 0);

  // finally, close subfile
  status = close (fd);
  TEST_ASSERT_EQUAL (status, 0);
}

static void test_read_write (void)
{
  int status, fd;

  fd = open ("X", O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU);
  TEST_ASSERT_UNEQUAL (fd, -1);

  status = write (fd, "100", 4);
  TEST_ASSERT_EQUAL (status, 4);

  status = close (fd);
  TEST_ASSERT_EQUAL (status, 0);

  fd = open ("X", O_RDONLY);
  TEST_ASSERT_UNEQUAL (fd, -1);

  char buffer[6];
  status = read (fd, buffer, 2);
  TEST_ASSERT_EQUAL (status, 2);

  status = read (fd, &buffer[2], 4);
  TEST_ASSERT_EQUAL (status, 2);

  status = read (fd, &buffer, 1);
  TEST_ASSERT_EQUAL (status, 0);

  status = close (fd);
  TEST_ASSERT_EQUAL (status, 0);

  status = strcmp ("100", buffer);
  TEST_ASSERT_EQUAL (status, 0);

  status = unlink ("X");
  TEST_ASSERT_EQUAL (status, 0);
}

static void test_cwd (void)
{
  char *cwd = get_current_dir_name ();
  TEST_ASSERT_UNEQUAL (cwd, 0);
  int len = strlen (cwd);
  free (cwd);

  char *buffer = (char*)malloc (len + 1);
  memset (buffer, 0, len + 1);
  cwd = getcwd (buffer, len);
  TEST_ASSERT_EQUAL (cwd, 0);
  cwd = getcwd (buffer, len + 1);
  TEST_ASSERT_UNEQUAL (cwd, 0);
  free (buffer);

  char tmp[PATH_MAX + 1];
  cwd = getwd (tmp);
  TEST_ASSERT_UNEQUAL (cwd, 0);
}

static void test_chdir (void)
{
  int status, fd;

  // verify target directory does not exist
  status = chdir ("X");
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, ENOENT);

  // create directory
  status = mkdir ("X", S_IRWXU);
  TEST_ASSERT_EQUAL (status, 0);

  // create subfile.
  fd = open ("X/A", O_CREAT | O_TRUNC | O_RDWR, S_IRWXU);
  TEST_ASSERT_UNEQUAL (status, -1);

  status = close (fd);
  TEST_ASSERT_EQUAL (status, 0);

  // go in target directory
  status = chdir ("X");
  TEST_ASSERT_EQUAL (status, 0);

  // verify we can open file
  fd = open ("A", O_RDONLY);
  TEST_ASSERT_UNEQUAL (fd, -1);
  status = close (fd);
  TEST_ASSERT_EQUAL (status, 0);

  // go up
  status = chdir ("..");
  TEST_ASSERT_EQUAL (status, 0);

  // verify we can open file
  fd = open ("X/A", O_RDONLY);
  TEST_ASSERT_UNEQUAL (fd, -1);
  status = close (fd);
  TEST_ASSERT_EQUAL (status, 0);

  // cleanup
  status = unlink ("X/A");
  TEST_ASSERT_EQUAL (status, 0);
  status = rmdir ("X");
  TEST_ASSERT_EQUAL (status, 0);
}

void test_unlinkat ()
{
  // create file /tmp/U
  int fd = open ("/tmp/U", O_CREAT | O_TRUNC | O_RDWR, S_IRWXU);
  TEST_ASSERT_UNEQUAL (fd, -1);

  // Open directory
  int fddir = open ("/tmp", O_RDONLY, 0);
  TEST_ASSERT (fddir > 0);

  // Delete file /tmp/U
  int status = unlinkat (fddir, "U", 0);
  TEST_ASSERT_EQUAL (status, 0);

  // Verify non existance of file /tmp/U
  int fd2 = open ("/tmp/U", O_RDONLY);
  TEST_ASSERT_EQUAL (fd2, -1);
  TEST_ASSERT_EQUAL (errno, ENOENT);

  status = close (fd);
  TEST_ASSERT_EQUAL (status, 0);

  // Re try using CWD
  // cd to /tmp
  status = chdir ("/tmp");
  TEST_ASSERT_EQUAL (status, 0);
  status = fchdir (fddir);
  TEST_ASSERT_EQUAL (status, 0);
  status = close (fddir);
  TEST_ASSERT_EQUAL (status, 0);
  // create file /tmp/U
  fd = open ("U", O_CREAT | O_TRUNC | O_RDWR, S_IRWXU);
  TEST_ASSERT_UNEQUAL (fd, -1);
  // Delete file /tmp/U
  status = unlinkat (AT_FDCWD, "U", 0);
  TEST_ASSERT_EQUAL (status, 0);
  status = close (fd);
  TEST_ASSERT_EQUAL (status, 0);

  // Retry with absolute path
  fd = open ("/tmp/U", O_CREAT | O_TRUNC | O_RDWR, S_IRWXU);
  TEST_ASSERT_UNEQUAL (fd, -1);
  // Delete file /tmp/U
  status = unlinkat (42, "/tmp/U", 0);
  TEST_ASSERT_EQUAL (status, 0);
  status = close (fd);
  TEST_ASSERT_EQUAL (status, 0);
}

void test_file_usage ()
{
  int fd = open ("F", O_CREAT | O_TRUNC | O_RDWR, S_IRWXU);
  TEST_ASSERT_UNEQUAL (fd, -1);
  int status = close (fd);
  TEST_ASSERT_EQUAL (status, 0);
}

static void test_pread_pwrite (void)
{
  int fd = open ("P", O_CREAT | O_TRUNC | O_RDWR, S_IRWXU);
  TEST_ASSERT_UNEQUAL (fd, -1);
  char buffer [1024];
  memset (buffer, 'A', sizeof (buffer));
  ssize_t w = pwrite (fd, buffer, sizeof (buffer), 0);
  TEST_ASSERT_EQUAL (w, sizeof (buffer));
  ssize_t p = lseek (fd, 0, SEEK_CUR);
  w = pread (fd, buffer, sizeof (buffer) / 2,  sizeof (buffer) / 2);
  TEST_ASSERT_EQUAL (w, sizeof (buffer) / 2);
  ssize_t p2 = lseek (fd, 0, SEEK_CUR);
  TEST_ASSERT_EQUAL (p, p2);
  int status = close (fd);
  TEST_ASSERT_EQUAL (status, 0);
  status = unlink ("P");
  TEST_ASSERT_EQUAL (status, 0);
}

void test_fsync ()
{
  int fd = open ("FS", O_CREAT | O_TRUNC | O_RDWR, S_IRWXU);
  TEST_ASSERT_UNEQUAL (fd, -1);
  int status = fsync (fd);
  TEST_ASSERT_EQUAL (status, 0);
  status = close (fd);
  TEST_ASSERT_EQUAL (status, 0);
}

int main (int argc, char *argv[])
{
  test_file_usage ();
  test_open_exclusive ();
  test_stat ();
  test_exists ();
  test_file_remove ();
  test_create_dir ();
  test_remove_dir ();
  test_subfile ();
  test_read_write ();
  test_cwd ();
  test_chdir ();
  test_unlinkat ();
  test_pread_pwrite ();
  test_fsync ();

  return 0;
}
