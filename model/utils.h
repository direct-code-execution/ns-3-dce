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
class Node;

// Little hack in order to have a context usable when disposing the Task Manager and the hidden goal is to flush the open FILEs.
extern Thread *gDisposingThreadContext;

/**
Create directory
*/
void UtilsEnsureDirectoryExists (std::string realPath);

/**
Will create all the directories in the path successively
*/
void UtilsEnsureAllDirectoriesExist (std::string realPath);
/**
\return "files-<nodeId>/path"
*/
std::string UtilsGetRealFilePath (std::string path);

/*
Same as UtilsGetRealFilePath excepts that it creates files-<nodeId> if it does not exist
*/
std::string UtilsGetAbsRealFilePath (uint32_t node, std::string path);

/**
\return absolute path from the node point of view.
*/
std::string UtilsGetVirtualFilePath (std::string path);
uint32_t UtilsGetNodeId (void);

/**
 * Get time of the node. To use with UtilsGetNodeId()
 */
Time UtilsNodeTime(uint32_t nodeId);
Ptr<Node> UtilsGetNode(uint32_t nodeId);

Thread * Current (void);
bool HasPendingSignal (void);
Time UtilsTimeToSimulationTime (Time time);

/**
* \brief translate ns3 time to epoch defined by GlobalValue "SimulationTimeBase"
* \see SimulationTimeBase
*/
Time UtilsSimulationTimeToTime (Time time);


/**
* \brief Converts ns3 Time to linux timeval structure
* \see UtilsTimevalToTime
*/
struct timeval UtilsTimeToTimeval (Time time);
struct timespec UtilsTimeToTimespec (Time time);

Time UtilsTimespecToTime (struct timespec tm);
Time UtilsTimevalToTime (struct timeval tv);
Time UtilsTimevalToTime (const struct timeval *tv);
void UtilsSendSignal (Process *process, int signum);
void UtilsDoSignal (void);
int UtilsAllocateFd (void);

/**
 * \brief Little hack to advance time when detecting a possible infinite loop.
 */
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


/* helpful to get consistent interface names since ns3 has no convention yet */
std::string UtilsGenerateIfNameFromIndex(uint32_t i);

#define MAX_FDS 1024

/* there must be a 'fd' variable in the outer scope */
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

/**
 * \param args should be a member of UnixFd
 * \return -1 in case of error
 */
#define OPENED_FD_METHOD(rettype, args) OPENED_FD_METHOD_ERR (-1, rettype, args)

} // namespace ns3

#endif /* UTILS_H */
