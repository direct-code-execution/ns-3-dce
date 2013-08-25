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
#include <libio.h>
#include <string.h>

NS_LOG_COMPONENT_DEFINE ("DceStdio");

using namespace ns3;

namespace {

struct my_IO_jump_t
{
  size_t dummy0;
  size_t dummy1;
  void *functions[12];
  void *__read;
  void *__write;
  void *__seek;
  void *__close;
  void *__stat;
  void *__showmanyc;
  void *__imbue;
};
struct my_IO_FILE_plus
{
  _IO_FILE file;
  struct my_IO_jump_t *vtable;
};

ssize_t my_read (_IO_FILE *file, void *buffer, ssize_t size)
{
  ssize_t data_read = dce_read (file->_fileno, buffer, size);
  if (data_read == -1)
    {
      errno = Current ()->err;
    }
  return data_read;
}
ssize_t my_write (_IO_FILE *file, const void *buffer, ssize_t size)
{
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
off64_t my_seek (_IO_FILE *file, off64_t where, int whence)
{
  off64_t result = dce_lseek (file->_fileno, where, whence);
  if (result == -1)
    {
      errno = Current ()->err;
    }
  return result;
}
int my_close (_IO_FILE *file)
{
  int result = dce_close (file->_fileno);
  if (result == -1)
    {
      errno = Current ()->err;
    }
  return result;
}
int my_close_unconditional (_IO_FILE *file)
{
  return 0;
}
int my_write_unconditional (_IO_FILE *file)
{
  errno = EBADF;
  return -1;
}
off64_t my_seek_unconditional (_IO_FILE *file, off64_t where, int whence)
{
  return -1;
}
int my_stat (_IO_FILE *file, void *buf)
{
  int result = dce_fstat64 (file->_fileno, (struct stat64 *)buf);
  if (result == -1)
    {
      errno = Current ()->err;
    }
  return result;
}
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
  // no need to create or truncate. Just need to seek if needed.
  FILE *file = fopen ("/dev/null", mode);
  if (file == 0)
    {
      current->err = errno;
      return 0;
    }
  struct my_IO_FILE_plus *fp = (struct my_IO_FILE_plus *)file;
  static struct my_IO_jump_t vtable;
  memcpy (&vtable, fp->vtable, sizeof(struct my_IO_jump_t));
  vtable.__read = (void*)my_read;
  vtable.__write = (void*)my_write;
  vtable.__seek = (void*)my_seek;
  vtable.__close = (void*)my_close;
  vtable.__stat = (void*)my_stat;
  fp->vtable = &vtable;
  close (file->_fileno);
  file->_fileno = fildes;
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
  int oldFd = stream->_fileno;
  stream->_fileno = -1;
  stream = freopen ("/dev/null", mode, stream);
  if (stream == 0)
    {
      stream->_fileno = oldFd;
      current->err = errno;
      return 0;
    }
  struct my_IO_FILE_plus *fp = (struct my_IO_FILE_plus *)stream;
  static struct my_IO_jump_t vtable;
  memcpy (&vtable, fp->vtable, sizeof(struct my_IO_jump_t));
  vtable.__read = (void*)my_read;
  vtable.__write = (void*)my_write;
  vtable.__seek = (void*)my_seek;
  vtable.__close = (void*)my_close;
  vtable.__stat = (void*)my_stat;
  fp->vtable = &vtable;

  int fd = dce_open (path, mode_posix_flags (mode), ~0);
  if (fd == -1)
    {
      dce_close (oldFd);
      fclose (stream);
      current->err = errno;
      return 0;
    }
  dce_close (oldFd);
  stream->_fileno = fd;
  mode_setup (stream, fd, mode);
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
  struct my_IO_FILE_plus *fp = (struct my_IO_FILE_plus *)file;
  static struct my_IO_jump_t vtable;
  memcpy (&vtable, fp->vtable, sizeof(struct my_IO_jump_t));
  vtable.__close = (void*)my_close_unconditional;
  vtable.__write = (void*)my_write_unconditional;
  vtable.__seek = (void*)my_seek_unconditional;
  fp->vtable = &vtable;
  fclose (file);
  return 0;
}
int dce_fclose_onexec (FILE *file)
{
  // Note: it is important here not to call the Current function here
  // because we need to be able to run this function even if there is no context.
  // For example, this is why we have no call to NS_LOG_FUNCTION (Current () ...);
  struct my_IO_FILE_plus *fp = (struct my_IO_FILE_plus *)file;
  static struct my_IO_jump_t vtable;
  memcpy (&vtable, fp->vtable, sizeof(struct my_IO_jump_t));
  vtable.__close = (void*)my_close_unconditional;
  fp->vtable = &vtable;
  fclose (file);
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
                        __const char *__restrict __format, _G_va_list __ap)
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
						__const char *__restrict __format, _G_va_list __ap)
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
      char *tmp = (char*) dce_malloc (ret);

      if (tmp)
        {
          memcpy (tmp, res, ret);

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
