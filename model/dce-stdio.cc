#include "dce-stdio.h"
#include "dce-stdarg.h"
#include "dce-fcntl.h"
#include "dce-unistd.h"
#include "process.h"
#include "dce-manager.h"
#include "utils.h"
#include "unix-fd.h"
#include "system-wrappers.h"
#include "ns3/log.h"
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <sys/mman.h>

NS_LOG_COMPONENT_DEFINE ("SimuStdio");

using namespace ns3;


FILE *dce_fdopen(int fildes, const char *mode)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << fildes << mode);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  SystemWrappersEnable ();
  FILE *file = fdopen (fildes, mode);
  SystemWrappersDisable ();
  if (file == 0)
    {
      current->err = errno;
      return 0;
    }
  current->process->openStreams.push_back (file);
  return file;
}


FILE *dce_fopen(const char *path, const char *mode)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << path << mode);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  SystemWrappersEnable ();
  FILE *file = fopen (path, mode);
  SystemWrappersDisable ();
  if (file == 0)
    {
      current->err = errno;
      return 0;
    }
  current->process->openStreams.push_back (file);
  return file;
}
FILE *dce_freopen(const char *path, const char *mode, FILE *stream)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << path << mode << stream);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  SystemWrappersEnable ();
  FILE *file = freopen (path, mode, stream);
  SystemWrappersDisable ();
  if (file == 0)
    {
      current->err = errno;
      return 0;
    }
  return file;
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
  return error?EOF:0;
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
int dce_fclose(FILE *fp)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << fp);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();

  if (fp == *current->process->pstdout ||
      fp == *current->process->pstderr ||
      fp == *current->process->pstdin)
    {
      return 0;
    }

  remove_stream (fp);

  SystemWrappersEnable ();
  int status = fclose (fp);
  SystemWrappersDisable ();
  NS_LOG_DEBUG ("fclose=" << status << " errno=" << errno);
  if (status != 0)
    {
      current->err = errno;
      return status;
    }
  return status;
}
size_t dce_fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << ptr << size << nmemb << stream);
  NS_ASSERT (Current () != 0);
  SystemWrappersEnable ();
  // Note: I believe that fread does not set errno ever
  size_t status = fread (ptr, size, nmemb, stream);
  SystemWrappersDisable ();
  return status;
}
size_t dce_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << ptr << size << nmemb << stream);
  NS_ASSERT (Current () != 0);
  SystemWrappersEnable ();
  // Note: I believe that fwrite does not set errno ever
  size_t status = fwrite (ptr, size, nmemb, stream);
  SystemWrappersDisable ();
  return status;
}
int dce_fflush(FILE *stream)
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
	  SystemWrappersEnable ();
	  int status = fflush (*i);
	  SystemWrappersDisable ();
	  if (status != 0)
	    {
	      current->err = errno;
	      return status;
	    }
	}
    }
  else
    {
      SystemWrappersEnable ();
      int status = fflush (stream);
      SystemWrappersDisable ();
      if (status != 0)
	{
	  current->err = errno;
	  return status;
	}
    }
  return 0;
}
void dce_clearerr(FILE *stream)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << stream);
  NS_ASSERT (Current () != 0);
  SystemWrappersEnable ();
  clearerr (stream);
  SystemWrappersDisable ();
}
int dce_feof(FILE *stream)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << stream);
  NS_ASSERT (Current () != 0);
  SystemWrappersEnable ();
  int status = feof (stream);
  SystemWrappersDisable ();
  return status;
}
int dce_ferror(FILE *stream)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << stream);
  NS_ASSERT (Current () != 0);
  SystemWrappersEnable ();
  int status = ferror (stream);
  SystemWrappersDisable ();
  return status;
}
int dce_fileno(FILE *stream)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << stream);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  SystemWrappersEnable ();
  int status = fileno (stream);
  SystemWrappersDisable ();
  if (status == -1)
    {
      current->err = errno;
      return -1;
    }
  return status;
}

int dce_vfprintf(FILE *stream, const char *format, va_list ap)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << stream << format);
  NS_ASSERT (Current () != 0);
  SystemWrappersEnable ();
  // Note: I don't believe that this function sets errno
  int status = vfprintf (stream, format, ap);
  SystemWrappersDisable ();
  return status;
}
int dce_fputc(int c, FILE *stream)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << c << stream);
  NS_ASSERT (Current () != 0);
  SystemWrappersEnable ();
  // Note: I don't believe that this function sets errno
  int status = fputc (c, stream);
  SystemWrappersDisable ();
  return status;
}
int dce_fgetc(FILE *stream)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << stream);
  NS_ASSERT (Current () != 0);
  SystemWrappersEnable ();
  // Note: I don't believe that this function sets errno
  int status = fgetc (stream);
  SystemWrappersDisable ();
  return status;
}
int dce_fputs(const char *s, FILE *stream)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << s << stream);
  NS_ASSERT (Current () != 0);
  SystemWrappersEnable ();
  // Note: I don't believe that this function sets errno
  int status = fputs (s, stream);
  SystemWrappersDisable ();
  return status;
}
char* dce_fgets(char *s, int size, FILE *stream)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << s << size << stream);
  NS_ASSERT (Current () != 0);
  SystemWrappersEnable ();
  // Note: I don't believe that this function sets errno
  char *status = fgets (s, size, stream);
  SystemWrappersDisable ();
  return status;
}
int dce_ungetc(int c, FILE *stream)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << c << stream);
  NS_ASSERT (Current () != 0);
  SystemWrappersEnable ();
  // Note: I don't believe that this function sets errno
  int status = ungetc (c, stream);
  SystemWrappersDisable ();
  return status;  
}
int dce_fseek(FILE *stream, long offset, int whence)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << stream << offset << whence);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  SystemWrappersEnable ();
  int status = fseek (stream, offset, whence);
  SystemWrappersDisable ();
  if (status == -1)
    {
      current->err = errno;
      return -1;
    }
  return status;
}
long dce_ftell(FILE *stream)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << stream);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  SystemWrappersEnable ();
  long status = ftell (stream);
  SystemWrappersDisable ();
  if (status == -1)
    {
      current->err = errno;
      return -1;
    }
  return status;
}
int dce_fgetpos(FILE *stream, fpos_t *pos)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << stream << pos);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  SystemWrappersEnable ();
  int status = fgetpos (stream, pos);
  SystemWrappersDisable ();
  if (status == -1)
    {
      current->err = errno;
      return -1;
    }
  return status;
}
int dce_fsetpos(FILE *stream, const fpos_t *pos)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << stream << pos);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  SystemWrappersEnable ();
  int status = fsetpos (stream, pos);
  SystemWrappersDisable ();
  if (status == -1)
    {
      current->err = errno;
      return -1;
    }
  return status;
}

void dce_rewind(FILE *stream)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << stream);
  NS_ASSERT (Current () != 0);
  SystemWrappersEnable ();
  rewind (stream);
  SystemWrappersDisable ();
}
int dce_setvbuf(FILE *stream, char *buf, int mode, size_t size)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << stream << buf << mode << size);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  SystemWrappersEnable ();
  int status = setvbuf (stream, buf, mode, size);
  SystemWrappersDisable ();
  if (status == -1)
    {
      current->err = errno;
      return -1;
    }
  return status;
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
