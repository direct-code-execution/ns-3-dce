#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <list>
#include <sys/time.h>
#include <sys/stat.h>
#include "ns3/nstime.h"

#define GET_CURRENT(x)                                  \
  Thread * current;                                      \
  current = Current ();                                 \
  do {                                                  \
      NS_LOG_FUNCTION (current << UtilsGetNodeId () << x);  \
      NS_ASSERT (current != 0);                             \
    } while (false)
#define GET_CURRENT_NOLOG()                             \
  Thread * current;                                      \
  current = Current ();                                 \
  do {                                                  \
      NS_LOG_FUNCTION (current << UtilsGetNodeId ());       \
      NS_ASSERT (current != 0);                             \
    } while (false)


namespace ns3 {

class Thread;
class Process;

// Little hack in order to have a context usable when disposing the Task Manager and the hidden goal is to flush the open FILEs.
extern Thread *gDisposingThreadContext;

void UtilsEnsureDirectoryExists (std::string realPath);
void UtilsEnsureAllDirectoriesExist (std::string realPath);
std::string UtilsGetRealFilePath (std::string path);
std::string UtilsGetAbsRealFilePath (uint32_t node, std::string path);
std::string UtilsGetVirtualFilePath (std::string path);
uint32_t UtilsGetNodeId (void);
Thread * Current (void);
bool HasPendingSignal (void);
Time UtilsTimeToSimulationTime (Time time);
Time UtilsSimulationTimeToTime (Time time);
struct timeval UtilsTimeToTimeval (Time time);
struct timespec UtilsTimeToTimespec (Time time);
Time UtilsTimespecToTime (struct timespec tm);
Time UtilsTimevalToTime (struct timeval tv);
Time UtilsTimevalToTime (const struct timeval *tv);
void UtilsSendSignal (Process *process, int signum);
void UtilsDoSignal (void);
int UtilsAllocateFd (void);
// Little hack to advance time when detecting a possible infinite loop.
void UtilsAdvanceTime (Thread *current);
std::string GetTimeStamp ();
bool CheckExeMode (struct stat *st, uid_t uid, gid_t gid);
std::string FindExecFile (std::string root, std::string envPath, std::string fileName, uid_t uid, gid_t gid, int *errNo);
std::list<std::string> Split (std::string input, std::string sep);
void FdDecUsage (int fd);
bool CheckFdExists (Process* const p, int const fd, bool const opened);
int getRealFd (int fd, Thread *current);
std::string PathOfFd (int fd);
bool CheckShellScript (std::string fileName,
                       std::ostringstream &shellName, std::ostringstream &optionalArg);
char * seek_env (const char *name, char **array);
std::string UtilsGetCurrentDirName (void);


/** ns3 does not name interfaces but some applications using netlink expect consistent
 * interface naming hence DCE propose its scheme.
 */
std::string UtilsGenerateIfNameFromIndex(uint32_t i);

#define MAX_FDS 1024

#define OPENED_FD_METHOD_ERR(errCode, rettype, args) \
  std::map < int,FileUsage * > ::iterator it = current->process->openFiles.find (fd); \
  if (current->process->openFiles.end () == it) \
    { \
      current->err = EBADF; \
      return (rettype) errCode; \
    } \
  FileUsage *fu = it->second; \
  if (fu->IsClosed ()) \
    { \
      current->err = EBADF; \
      return (rettype) errCode; \
    } \
  UnixFd *unixFd =  fu->GetFileInc (); \
  rettype retval = unixFd->args; \
  if (fu && fu->DecUsage ()) \
    { \
      current->process->openFiles.erase (fd); \
      delete fu; \
      fu = 0; \
    } \
\
  return retval;

#define OPENED_FD_METHOD(rettype, args) OPENED_FD_METHOD_ERR (-1, rettype, args)

} // namespace ns3

#endif /* UTILS_H */
