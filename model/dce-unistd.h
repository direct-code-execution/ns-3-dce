
/* DO NOT MODIFY - GENERATED BY script */
#ifndef DCE_HEADER_UNISTD_H
#define DCE_HEADER_UNISTD_H
// TODO add extern "C" ?
#include <unistd.h>
// TODO temporary hack
#define __restrict__

#ifdef __cplusplus
extern "C" {
#endif
                 ssize_t dce_read (int __fd,void * __buf,size_t __nbytes);

 ssize_t dce_write (int __fd,void const * __buf,size_t __n);

 unsigned int dce_sleep (unsigned int __seconds);

 int dce_usleep (__useconds_t __useconds);

 __pid_t dce_getpid ();

 __pid_t dce_getppid ();

 __uid_t dce_getuid ();

 __uid_t dce_geteuid ();

 int dce_setuid (__uid_t __uid);

 int dce_setgid (__gid_t __gid);

 int dce_seteuid (__uid_t __uid);

 int dce_setegid (__gid_t __gid);

 int dce_setreuid (__uid_t __ruid,__uid_t __euid);

 int dce_setregid (__gid_t __rgid,__gid_t __egid);

 int dce_setresuid (__uid_t __ruid,__uid_t __euid,__uid_t __suid);

 int dce_setresgid (__gid_t __rgid,__gid_t __egid,__gid_t __sgid);

 int dce_dup (int __fd);

 int dce_dup2 (int __fd,int __fd2);

 int dce_close (int __fd);

 int dce_unlink (char const * __name);

 int dce_rmdir (char const * __path);

 int dce_isatty (int __fd);

 char * dce_getcwd (char * __buf,size_t __size);

 char * dce_getwd (char * __buf);

 char * dce_get_current_dir_name ();

 int dce_chdir (char const * __path);

 int dce_fchdir (int __fd);

 __pid_t dce_fork ();

 int dce_execv (char const * __path,char * const * __argv);

 int dce_execl (char const * __path,char const * __arg,... );

 int dce_execve (char const * __path,char * const * __argv,char * const * __envp);

 int dce_execvp (char const * __file,char * const * __argv);

 int dce_execlp (char const * __file,char const * __arg,... );

 int dce_execle (char const * __path,char const * __arg,... );

 int dce_truncate (char const * __file,__off_t __length);

 int dce_ftruncate (int __fd,__off_t __length);

 int dce_ftruncate64 (int __fd,__off64_t __length);


 char * dce_ttyname (int __fd);

 void * dce_sbrk (intptr_t __delta);

 int dce_getpagesize ();

 __gid_t dce_getgid ();

 __gid_t dce_getegid ();

 int dce_gethostname (char * __name,size_t __len);

 __pid_t dce_getpgrp ();

 __off_t dce_lseek (int __fd,__off_t __offset,int __whence);

 __off64_t dce_lseek64 (int __fd,__off64_t __offset,int __whence);

 int dce_euidaccess (char const * __name,int __type);

 int dce_eaccess (char const * __name,int __type);

 int dce_access (char const * __name,int __type);

 int dce_pipe (int * __pipedes);



 ssize_t dce_pread (int __fd,void * __buf,size_t __nbytes,__off_t __offset);

 ssize_t dce_pwrite (int __fd,void const * __buf,size_t __n,__off_t __offset);

 int dce_daemon (int __nochdir,int __noclose);

 unsigned int dce_alarm (unsigned int __seconds);

 ssize_t dce_readlink (__restrict__ char const * __path,__restrict__ char * __buf,size_t __len);

 int dce_chown (char const * __file,__uid_t __owner,__gid_t __group);

 int dce_fsync (int __fd);

 int dce_unlinkat (int __fd,char const * __name,int __flag);


#ifdef __cplusplus
}
#endif
#endif
