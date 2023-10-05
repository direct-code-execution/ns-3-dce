#include "dce-stdio.h"
#include "dce-stdarg.h"
#include "dce-fcntl.h"
#include "dce-unistd.h"
#include "dce-stdlib.h"
#include "sys/dce-stat.h"
#include "process.h"
#include "utils.h"
#include "unix-fd.h"
#include "ns3/log.h"
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <string.h>
#include <cstdarg>

NS_LOG_COMPONENT_DEFINE ("DceStdio");

using namespace ns3;

namespace {

struct my_cookie
{
  int _fileno;
  off64_t _offset;
};

ssize_t my_read (void *c, char *buffer, size_t size)
{
  struct my_cookie *file = (struct my_cookie *)c;
  ssize_t data_read = dce_read (file->_fileno, buffer, size);
  if (data_read == -1)
    {
      errno = Current ()->err;
    }
  return data_read;
}
ssize_t my_write (void *c, const char *buffer, size_t size)
{
  struct my_cookie *file = (struct my_cookie *)c;
  ssize_t data_written = dce_write (file->_fileno, buffer, size);
  if (data_written == -1)
    {
      errno = Current ()->err;
    }
  if (file->_offset >= 0)
    {
      file->_offset += data_written;
    }
  return data_written;
}
int my_seek(void *c, off64_t *where, int whence)
{
  struct my_cookie *file = (struct my_cookie *)c;
  off64_t result = dce_lseek (file->_fileno, *where, whence);
  if (result == -1)
    {
      errno = Current ()->err;
      return -1;
    }
  file->_offset = result;
  *where = result;
  return 0;
}
int my_close(void *c)
{
  struct my_cookie *file = (struct my_cookie *)c;
  int result = dce_close (file->_fileno);
  if (result == -1)
    {
      errno = Current ()->err;
    }
  free(file);
  return result;
}

cookie_io_functions_t my_func = {
  .read = my_read,
  .write = my_write,
  .seek = my_seek,
  .close = my_close,
};

bool mode_seek_start (const char *mode)
{
  return *mode != 'a';
}
bool mode_truncate (const char *mode)
{
  return *mode == 'w';
}
bool mode_create (const char *mode)
{
  return *mode != 'r';
}
bool mode_valid (const char *mode)
{
  while (*mode != 0)
    {
      switch (*mode)
        {
        case 'a':
        case 'r':
        case 'w':
        case '+':
        case 'b':
          break;
        default:
          return false;
          break;
        }
      mode++;
    }
  return true;
}
int mode_posix_flags (const char *mode)
{
  int mode_flag = 0;
  int posix_flags = 0;
  switch (*mode)
    {
    case 'r':
      mode_flag |= O_RDONLY;
      break;
    case 'w':
      mode_flag |= O_WRONLY;
      posix_flags |= O_CREAT | O_TRUNC;
      break;
    case 'a':
      mode_flag |= O_WRONLY;
      posix_flags |= O_CREAT | O_APPEND;
      break;
    }
  mode++;
  while (*mode != 0)
    {
      if (*mode == '+')
        {
          mode_flag = O_RDWR;
        }
      mode++;
    }
  posix_flags |= mode_flag;
  return posix_flags;
}
void mode_setup (FILE *file, int fd, const char *mode)
{
  if (mode_seek_start (mode))
    {
      dce_lseek (fd, 0, SEEK_SET);
      dce_fseek (file, 0, SEEK_SET);
    }
  else
    {
      dce_lseek (fd, 0, SEEK_END);
      dce_fseek (file, 0, SEEK_END);
    }
}

} // namespace

FILE * dce_fdopen (int fildes, const char *mode)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << fildes << mode);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  struct my_cookie *fp = (struct my_cookie*)malloc(sizeof(struct my_cookie));
  fp->_fileno = fildes;
  FILE *file = fopencookie(fp, mode, my_func);
  current->process->openStreams.push_back (file);
  dce_fseek (file, dce_lseek (fildes, 0, SEEK_CUR), SEEK_SET);

  return file;
}

FILE * dce_fopen64 (const char *path, const char *mode)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << path << mode);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  if (!mode_valid (mode))
    {
      current->err = EINVAL;
      return 0;
    }
  int fd = dce_open (path, mode_posix_flags (mode) | O_LARGEFILE, 0666 & ~(current->process->uMask));
  if (fd == -1)
    {
      current->err = errno;
      return 0;
    }
  FILE *file = dce_fdopen (fd, mode);
  if (file == 0)
    {
      return 0;
    }
  mode_setup (file, fd, mode);
  return file;
}
FILE * dce_fopen (const char *path, const char *mode)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << path << mode);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  if (!mode_valid (mode))
    {
      current->err = EINVAL;
      return 0;
    }
  int fd = dce_open (path, mode_posix_flags (mode), 0666 & ~(current->process->uMask));
  if (fd == -1)
    {
      current->err = errno;
      return 0;
    }
  FILE *file = dce_fdopen (fd, mode);
  if (file == 0)
    {
      return 0;
    }
  mode_setup (file, fd, mode);
  return file;
}
// FIXME: Ugly but less code movement.
static void remove_stream (FILE *fp);
FILE * dce_freopen (const char *path, const char *mode, FILE *stream)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << path << mode << stream);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  if (!mode_valid (mode))
    {
      current->err = EINVAL;
      return 0;
    }
  if (path == NULL) {
      // Just change mode
      // Create a copy of the fd behind stream
      int oldFdCopy = dup(fileno(stream));
      // Close the old
      fclose(stream);
      remove_stream(stream);
      // And create a new dce_fdopen from the copied fd
      // // And create a new dce_fdopen from the copied fd
      stream = dce_fdopen(oldFdCopy, mode);
      return stream;
  }
  if (stream) {
      fclose(stream);
      remove_stream(stream);
  }

  stream = dce_fopen(path, mode);

  return stream;
}
int dce_fcloseall (void)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);
  struct Process *process = Current ()->process;
  bool error = false;
  for (uint32_t i =  0; i < process->openStreams.size (); i++)
    {
      int status = dce_fclose (process->openStreams[i]);
      if (status != 0)
        {
          error = true;
        }
    }
  return error ? EOF : 0;
}
static void
remove_stream (FILE *fp)
{
  Thread *current = Current ();
  bool found = false;
  for (std::vector<FILE*>::iterator  i = current->process->openStreams.begin ();
       i != current->process->openStreams.end (); ++i)
    {
      if (*i == fp)
        {
          current->process->openStreams.erase (i);
          found = true;
          break;
        }
    }
  if (!found)
    {
      // not found
      NS_FATAL_ERROR ("invalid FILE * closed=" << fp);
    }
}
int dce_fclose_unconditional (FILE *file)
{
  // Note: it is important here not to call the Current function here
  // because we need to be able to run this function even if there is no context.
  // For example, this is why we have no call to NS_LOG_FUNCTION (Current () ...);
  // TODO
  //fclose(file);
  // Maybee dce_close(fileno(file)) ?
  return 0;
}
int dce_fclose_onexec (FILE *file)
{
  // Note: it is important here not to call the Current function here
  // because we need to be able to run this function even if there is no context.
  // For example, this is why we have no call to NS_LOG_FUNCTION (Current () ...);
  // TODO: fclose here might cause dce_write and will cause dce_close which calls Current()...
  // but it works in the tests now.
  fclose(file);
  return 0;
}
int dce_fclose (FILE *fp)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << fp);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();

  if ((current->process->pstdout && fp == *current->process->pstdout)
      || (current->process->pstdin && fp == *current->process->pstderr)
      || (current->process->pstdin && fp == *current->process->pstdin))
    {
      // TODO: BUG: This is not right
      // fcloseall says:
      // The standard streams, stdin, stdout, and stderr are also closed.
      return 0;
    }

  remove_stream (fp);

  int status = fclose (fp);
  NS_LOG_DEBUG ("fclose=" << status << " errno=" << errno);
  if (status != 0)
    {
      current->err = errno;
      return status;
    }
  return status;
}
size_t dce_fread (void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << ptr << size << nmemb << stream);
  NS_ASSERT (Current () != 0);
  // Note: I believe that fread does not set errno ever
  size_t status = fread (ptr, size, nmemb, stream);
  return status;
}
size_t dce_fwrite (const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << ptr << size << nmemb << stream);
  NS_ASSERT (Current () != 0);
  // Note: I believe that fwrite does not set errno ever
  size_t status = fwrite (ptr, size, nmemb, stream);
  return status;
}
int dce_fflush (FILE *stream)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << stream);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  if (stream == 0)
    {
      // only flush the streams of the current process
      for (std::vector<FILE *>::const_iterator i = current->process->openStreams.begin ();
           i != current->process->openStreams.end (); ++i)
        {
          int status = fflush (*i);
          if (status != 0)
            {
              current->err = errno;
              return status;
            }
        }
    }
  else
    {
      int status = fflush (stream);
      if (status != 0)
        {
          current->err = errno;
          return status;
        }
    }
  return 0;
}
void dce_clearerr (FILE *stream)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << stream);
  NS_ASSERT (Current () != 0);
  clearerr (stream);
}
int dce_feof (FILE *stream)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << stream);
  NS_ASSERT (Current () != 0);
  int status = feof (stream);
  return status;
}
int dce_ferror (FILE *stream)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << stream);
  NS_ASSERT (Current () != 0);
  int status = ferror (stream);
  return status;
}
int dce_fileno (FILE *stream)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << stream);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();

  // UGLY: FIXME: Use current->process->openStreams instead
  if (current->process->pstdin && stream == *current->process->pstdin) {
      return 0;
  } else if (current->process->pstdout && stream == *current->process->pstdout) {
      return 1;
  } else if (current->process->pstderr && stream == *current->process->pstderr) {
      return 2;
  }

  // FIXME: Handle fopencookie things to. We need to detect those FILE*
  // and return cookie->_fileno instead... But how?

  int status = fileno (stream);
  if (status == -1)
    {
      current->err = errno;
      return -1;
    }
  return status;
}

// stdio.h
int dce_printf (const char *format, ...)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << format);
  NS_ASSERT (Current () != 0);

  va_list vl;
  va_start (vl, format);
  int status = vfprintf (*Current ()->process->pstdout, format, vl);
  va_end (vl);
  return status;
}
// stdarg.h
int dce_vprintf (const char *format, va_list ap)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << format);
  NS_ASSERT (Current () != 0);
  // Note: I don't believe that this function sets errno
  int status = vfprintf (*Current ()->process->pstdout, format, ap);
  return status;
}

int dce_getchar (void)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);
  return fgetc (*Current ()->process->pstdin);
}

int dce__IO_getc (FILE *stream)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);
  return fgetc (stream);
}

int dce_putchar (int __c)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << (char)__c);
  NS_ASSERT (Current () != 0);
  return fputc (__c, *Current ()->process->pstdout);
}

int dce__IO_putc (int __c, FILE *__stream)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << (char)__c);
  NS_ASSERT (Current () != 0);
  return fputc (__c, __stream);
}

int dce_puts (const char *__s)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << __s);
  NS_ASSERT (Current () != 0);
  int ret = fputs (__s, *Current ()->process->pstdout);
  if (ret < 0)
    {
      return ret;
    }
  fputc ('\n', *Current ()->process->pstdout);
  return ret;
}

int dce_fputc (int c, FILE *stream)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << c << stream);
  NS_ASSERT (Current () != 0);
  // Note: I don't believe that this function sets errno
  int status = fputc (c, stream);
  return status;
}
int dce_fgetc (FILE *stream)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << stream);
  NS_ASSERT (Current () != 0);
  // Note: I don't believe that this function sets errno
  int status = fgetc (stream);
  return status;
}
int dce_fputs (const char *s, FILE *stream)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << s << stream);
  NS_ASSERT (Current () != 0);
  // Note: I don't believe that this function sets errno
  int status = fputs (s, stream);
  return status;
}
char* dce_fgets (char *s, int size, FILE *stream)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << s << size << stream);
  NS_ASSERT (Current () != 0);
  // Note: I don't believe that this function sets errno
  char *status = fgets (s, size, stream);
  return status;
}
int dce_ungetc (int c, FILE *stream)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << c << stream);
  NS_ASSERT (Current () != 0);
  // Note: I don't believe that this function sets errno
  int status = ungetc (c, stream);
  return status;
}
int dce_fseek (FILE *stream, long offset, int whence)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << stream << offset << whence);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  int status = fseek (stream, offset, whence);
  if (status == -1)
    {
      current->err = errno;
      return -1;
    }
  return status;
}
int dce_fseeko (FILE *stream, off_t offset, int whence)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << stream << offset << whence);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  int status = fseeko (stream, offset, whence);
  if (status == -1)
    {
      current->err = errno;
      return -1;
    }
  return status;
}
long dce_ftell (FILE *stream)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << stream);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  long status = ftell (stream);
  if (status == -1)
    {
      current->err = errno;
      return -1;
    }
  return status;
}
off_t dce_ftello (FILE *stream)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << stream);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  off_t status = ftello (stream);
  if (status == -1)
    {
      current->err = errno;
      return -1;
    }
  return status;
}
int dce_fgetpos (FILE *stream, fpos_t *pos)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << stream << pos);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  int status = fgetpos (stream, pos);
  if (status == -1)
    {
      current->err = errno;
      return -1;
    }
  return status;
}
int dce_fsetpos (FILE *stream, const fpos_t *pos)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << stream << pos);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  int status = fsetpos (stream, pos);
  if (status == -1)
    {
      current->err = errno;
      return -1;
    }
  return status;
}

void dce_rewind (FILE *stream)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << stream);
  NS_ASSERT (Current () != 0);
  rewind (stream);
}
int dce_setvbuf (FILE *stream, char *buf, int mode, size_t size)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << stream << ((void*)buf) << mode << size);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  int status = setvbuf (stream, buf, mode, size);
  if (status == -1)
    {
      current->err = errno;
      return -1;
    }
  return status;
}

void dce_setbuf (FILE *stream, char *buf)
{
  dce_setvbuf (stream, buf, buf ? _IOFBF : _IONBF, BUFSIZ);
}

void dce_setbuffer (FILE *stream, char *buf, size_t size)
{
  dce_setvbuf (stream, buf, buf ? _IOFBF : _IONBF, size);
}

void dce_setlinebuf (FILE *stream)
{
  dce_setvbuf (stream, (char *) NULL, _IOLBF, 0);
}

int dce_remove (const char *pathname)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << pathname);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  if (std::string (pathname) == "")
    {
      current->err = ENOENT;
      return -1;
    }
  std::string fullpath = UtilsGetRealFilePath (pathname);
  int status = ::remove (fullpath.c_str ());
  if (status == -1)
    {
      current->err = errno;
    }
  return status;
}

void dce_perror (const char *s)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);

  fprintf (*Current ()->process->pstderr, "%s: %s\n", s, strerror (*__errno_location ()));
}

int dce___printf_chk (int __flag, __const char *__restrict __format, ...)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);

  va_list ap;
  va_start (ap, __format);
  int retval = vfprintf (*Current ()->process->pstdout, __format, ap);
  va_end (ap);
  return retval;
}

int dce___vfprintf_chk (FILE *__restrict __stream, int __flag,
                        __const char *__restrict __format, __gnuc_va_list __ap)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);

  return vfprintf (__stream, __format, __ap);
}

int dce___fprintf_chk (FILE *__restrict __stream, int __flag,
                       __const char *__restrict __format, ...)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);

  va_list ap;
  va_start (ap, __format);
  int retval = vfprintf (__stream, __format, ap);
  va_end (ap);
  return retval;
}

int dce___snprintf_chk (char *__restrict __s, size_t __n, int __flag,
                        size_t __slen, __const char *__restrict __format, ...)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);

  va_list ap;
  va_start (ap, __format);
  int retval = vsnprintf (__s, __n, __format, ap);
  va_end (ap);
  return retval;
}
int dce___vsnprintf_chk (char *__restrict __s, size_t __n, int __flag,
						size_t __slen,
						__const char *__restrict __format, __gnuc_va_list __ap)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);

  int retval = vsnprintf (__s, __n, __format, __ap);
  return retval;
}
void dce___fpurge (FILE *stream)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << stream);
  NS_ASSERT (Current () != 0);
  __fpurge (stream);
}
size_t dce___fpending (FILE *stream)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);
  size_t ret = __fpending (stream);
  return ret;
}
int dce_asprintf (char **strp, const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);

  return dce_vasprintf (strp, fmt, ap);
}

int dce_vasprintf (char **strp, const char *fmt, va_list ap)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);

  char *res = 0;
  int ret = 0;

  if (!strp || !fmt)
    {
      Current ()->err = ENOMEM;
      return -1;
    }

  ret = vasprintf (&res, fmt, ap);

  if (ret > 0)
    {
      char *tmp = (char*) dce_malloc (ret + 1);

      if (tmp)
        {
          memcpy (tmp, res, ret + 1);

          *strp = tmp;
          free (res);

          return ret;
        }
      free (res);
      Current ()->err = ENOMEM;
      return -1;
    }
  Current ()->err = errno;

  return ret;
}
int dce_vsnprintf (char *s, size_t si, const char *f, va_list ap)
{
  return ::vsnprintf (s, si, f, ap);
}
