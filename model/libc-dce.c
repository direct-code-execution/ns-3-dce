#define _GNU_SOURCE 1
#include "libc-dce.h"
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <locale.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <ctype.h>
#include <time.h>
#include "sys/dce-socket.h"
#include "dce-unistd.h"
#include "dce-netdb.h"
#include "dce-pthread.h"
#include "dce-stdio.h"
#include "dce-stdarg.h"
#include "dce-errno.h"
#include "dce-fcntl.h"
#include "sys/dce-time.h"
#include "sys/dce-ioctl.h"
#include "dce-sched.h"
#include "dce-poll.h"
#include "dce-signal.h"
#include "dce-stdlib.h"
#include "dce-time.h"
#include "sys/dce-mman.h"
#include "sys/dce-stat.h"
#include "sys/dce-select.h"
#include "dce-semaphore.h"
#include "arpa/dce-inet.h"
#include "dce-cxa.h"
#include "dce-string.h"
#include "dce-global-variables.h"
#include "sys/dce-timerfd.h"
#include "dce-random.h"
#include "net/dce-if.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int (*__gxx_personality_v0) (int a, int b, 
				    unsigned c, 
				    struct _Unwind_Exception *d, 
				    struct _Unwind_Context *e);
extern int __xpg_strerror_r (int __errnum, char *__buf, size_t __buflen);


void libc_dce (struct Libc *libc)
{
  libc->global_variables_setup = dce_global_variables_setup;
  libc->strerror_fn = strerror;
  libc->gnu_strerror_r_fn = strerror_r;
  libc->xsi_strerror_r_fn = __xpg_strerror_r;
  libc->atexit_fn = dce_atexit;
  libc->__cxa_finalize_fn = dce__cxa_finalize;
  libc->__cxa_atexit_fn = dce__cxa_atexit;
  libc->__gxx_personality_v0_fn = __gxx_personality_v0;
  libc->newlocale_fn = newlocale;
  libc->uselocale_fn = uselocale;
  libc->wctob_fn = wctob;
  libc->btowc_fn = btowc;
  libc->wctype_l_fn = wctype_l;
  libc->htonl_fn = htonl;
  libc->htons_fn = htons;
  libc->ntohl_fn = ntohl;
  libc->ntohs_fn = ntohs;
  libc->socket_fn = dce_socket;
  libc->getsockname_fn = dce_getsockname;
  libc->getpeername_fn = dce_getpeername;
  libc->bind_fn = dce_bind;
  libc->read_fn = dce_read;
  libc->connect_fn = dce_connect;
  libc->write_fn = dce_write;
  libc->writev_fn = dce_writev;
  libc->memset_fn = memset;
  libc->memcpy_fn = memcpy;
  libc->memcmp_fn = memcmp;
  libc->memmove_fn = memmove;
  libc->strcpy_fn = strcpy;
  libc->strncpy_fn = strncpy;
  libc->strcat_fn = strcat;
  libc->strncat_fn = strncat;
  libc->strcmp_fn = strcmp;
  libc->strncmp_fn = strncmp;
  libc->strlen_fn = strlen;
  libc->strchr_fn = strchr;
  libc->strrchr_fn = strrchr;
  libc->strcasecmp_fn = strcasecmp;
  libc->strncasecmp_fn = strncasecmp;
  libc->strdup_fn = dce_strdup;
  libc->strndup_fn = dce_strndup;
  libc->sleep_fn = dce_sleep;
  libc->fopen_fn = dce_fopen;
  libc->fdopen_fn = dce_fdopen;
  libc->freopen_fn = dce_freopen;
  libc->fclose_fn = dce_fclose;
  libc->fcloseall_fn = dce_fcloseall;
  libc->fread_fn = dce_fread;
  libc->fwrite_fn = dce_fwrite;
  libc->fflush_fn = dce_fflush;
  libc->fseek_fn = dce_fseek;
  libc->ftell_fn = dce_ftell;
  libc->fgetpos_fn = dce_fgetpos;
  libc->fsetpos_fn = dce_fsetpos;
  libc->ferror_fn = dce_ferror;
  libc->feof_fn = dce_feof;
  libc->fileno_fn = dce_fileno;
  libc->clearerr_fn = dce_clearerr;
  libc->setvbuf_fn = dce_setvbuf;
  libc->rewind_fn = dce_rewind;
  libc->vfprintf_fn = dce_vfprintf;
  libc->vsprintf_fn = vsprintf;
  libc->vsnprintf_fn = vsnprintf;
  libc->fputc_fn = dce_fputc;
  libc->fputs_fn = dce_fputs;
  libc->fgetc_fn = dce_fgetc;
  libc->fgets_fn = dce_fgets;
  libc->ungetc_fn = dce_ungetc;

  libc->fcntl_fn = dce_fcntl;
  libc->nanosleep_fn = dce_nanosleep;

  libc->random_fn = dce_random;
  libc->rand_fn = dce_rand;
  libc->srandom_fn = dce_srandom;
  libc->srand_fn = dce_srand;
  libc->seed48_fn = dce_seed48;
  libc->drand48_fn = dce_drand48;
  libc->nrand48_fn = dce_nrand48;
  libc->lrand48_fn = dce_lrand48;
  libc->mrand48_fn = dce_mrand48;
  libc->erand48_fn = dce_erand48;
  libc->jrand48_fn = dce_jrand48;
  libc->srand48_fn = dce_srand48;
  libc->lcong48_fn = dce_lcong48;

  libc->drand48_r_fn = drand48_r;
  libc->erand48_r_fn = erand48_r;
  libc->lrand48_r_fn = lrand48_r;
  libc->nrand48_r_fn = nrand48_r;
  libc->mrand48_r_fn = mrand48_r;
  libc->jrand48_r_fn = jrand48_r;
  libc->srand48_r_fn = srand48_r;
  libc->seed48_r_fn = seed48_r;
  libc->lcong48_r_fn = lcong48_r;

  libc->__errno_location_fn = dce_get_errno;
  libc->getopt_r_fn = dce_getopt_r;
  libc->getopt_long_r_fn = dce_getopt_long_r;
  libc->getpid_fn = dce_getpid;
  libc->getppid_fn = dce_getppid;
  libc->getuid_fn = dce_getuid;
  libc->geteuid_fn = dce_geteuid;
  libc->setuid_fn = dce_setuid;
  libc->setgid_fn = dce_setgid;
  libc->seteuid_fn = dce_seteuid;
  libc->setegid_fn = dce_setegid;
  libc->setreuid_fn = dce_setreuid;
  libc->setregid_fn = dce_setregid;
  libc->setresuid_fn = dce_setresuid;
  libc->setresgid_fn = dce_setresgid;
  libc->inet_aton_fn = inet_aton;
  libc->inet_ntoa_fn = inet_ntoa;
  libc->inet_ntop_fn = dce_inet_ntop;
  libc->inet_network_fn = inet_network;
  libc->inet_makeaddr_fn = inet_makeaddr;
  libc->inet_lnaof_fn = inet_lnaof;
  libc->inet_netof_fn = inet_netof;
  libc->inet_addr_fn = inet_addr;
  libc->mmap64_fn = dce_mmap64;
  libc->munmap_fn = dce_munmap;
  libc->xstat_fn = dce_xstat;
  libc->lxstat_fn = dce_lxstat;
  libc->fxstat_fn = dce_fxstat;
  libc->xstat64_fn = dce_xstat64;
  libc->lxstat64_fn = dce_lxstat64;
  libc->fxstat64_fn = dce_fxstat64;
  libc->dup_fn = dce_dup;
  libc->dup2_fn = dce_dup2;
  libc->open_fn = dce_open;
  libc->close_fn = dce_close;
  libc->unlink_fn = dce_unlink;
  libc->remove_fn = dce_remove;
  libc->mkdir_fn = dce_mkdir;
  libc->rmdir_fn = dce_rmdir;
  libc->setsockopt_fn = dce_setsockopt;
  libc->getsockopt_fn = dce_getsockopt;
  libc->select_fn = dce_select;
  libc->listen_fn = dce_listen;
  libc->accept_fn = dce_accept;
  libc->shutdown_fn = dce_shutdown;
  libc->calloc_fn = dce_calloc;
  libc->malloc_fn = dce_malloc;
  libc->free_fn = dce_free;
  libc->realloc_fn = dce_realloc;
  libc->gettimeofday_fn = dce_gettimeofday;
  libc->time_fn = dce_time;
  libc->isatty_fn = dce_isatty;
  libc->send_fn = dce_send;
  libc->sendto_fn = dce_sendto;
  libc->sendmsg_fn = dce_sendmsg;
  libc->recv_fn = dce_recv;
  libc->recvfrom_fn = dce_recvfrom;
  libc->recvmsg_fn = dce_recvmsg;
  libc->ioctl_fn = dce_ioctl;
  libc->sched_yield_fn = dce_sched_yield;
  libc->poll_fn = dce_poll;
  libc->__cmsg_nxthdr_fn = __cmsg_nxthdr;
  libc->atoi_fn = atoi;
  libc->atol_fn = atol;
  libc->atoll_fn = atoll;
  libc->atof_fn = atof;
  libc->setitimer_fn = dce_setitimer;
  libc->getitimer_fn = dce_getitimer;
  libc->exit_fn = dce_exit;
  libc->signal_fn = dce_signal;
  libc->sigaction_fn = dce_sigaction;
  libc->sigemptyset_fn = sigemptyset;
  libc->sigfillset_fn = sigfillset;
  libc->sigaddset_fn = sigaddset;
  libc->sigdelset_fn = sigdelset;
  libc->sigismember_fn = sigismember;
  libc->strtol_fn = dce_strtol;
  libc->strtoll_fn = dce_strtoll;
  libc->strtoul_fn = dce_strtoul;
  libc->strtoull_fn = dce_strtoull;
  libc->strtod_fn = dce_strtod;
  libc->getcwd_fn = dce_getcwd;
  libc->getwd_fn = dce_getwd;
  libc->get_current_dir_name_fn = dce_get_current_dir_name;
  libc->chdir_fn = dce_chdir;
  libc->fchdir_fn = dce_fchdir;
  libc->localtime_fn = localtime;
  libc->strftime_fn = strftime;
  libc->__ctype_b_loc_fn = __ctype_b_loc;
  libc->gmtime_r_fn = gmtime_r;

  libc->pthread_create_fn = dce_pthread_create;
  libc->pthread_exit_fn = dce_pthread_exit;
  libc->pthread_self_fn = dce_pthread_self;
  libc->pthread_once_fn = dce_pthread_once;
  libc->pthread_getspecific_fn = dce_pthread_getspecific;
  libc->pthread_setspecific_fn = dce_pthread_setspecific;
  libc->pthread_key_create_fn = dce_pthread_key_create;
  libc->pthread_key_delete_fn = dce_pthread_key_delete;
  libc->pthread_mutex_destroy_fn = dce_pthread_mutex_destroy;
  libc->pthread_mutex_init_fn = dce_pthread_mutex_init;
  libc->pthread_mutex_lock_fn = dce_pthread_mutex_lock;
  libc->pthread_mutex_unlock_fn = dce_pthread_mutex_unlock;
  libc->pthread_mutex_trylock_fn = dce_pthread_mutex_trylock;
  libc->pthread_mutexattr_init_fn = dce_pthread_mutexattr_init;
  libc->pthread_mutexattr_destroy_fn = dce_pthread_mutexattr_destroy;
  libc->pthread_mutexattr_settype_fn = dce_pthread_mutexattr_settype;
  libc->pthread_cancel_fn = dce_pthread_cancel;
  libc->pthread_kill_fn = dce_pthread_kill;
  libc->pthread_join_fn = dce_pthread_join;
  libc->pthread_detach_fn = dce_pthread_detach;
  libc->sem_init_fn = dce_sem_init;
  libc->sem_destroy_fn = dce_sem_destroy;
  libc->sem_post_fn = dce_sem_post;
  libc->sem_wait_fn = dce_sem_wait;
  libc->sem_timedwait_fn = dce_sem_timedwait;
  libc->sem_trywait_fn = dce_sem_trywait;
  libc->sem_getvalue_fn = dce_sem_getvalue;
  libc->pthread_cond_destroy_fn = dce_pthread_cond_destroy;
  libc->pthread_cond_init_fn = dce_pthread_cond_init;
  libc->pthread_cond_broadcast_fn = dce_pthread_cond_broadcast;
  libc->pthread_cond_signal_fn = dce_pthread_cond_signal;
  libc->pthread_cond_timedwait_fn = dce_pthread_cond_timedwait;
  libc->pthread_cond_wait_fn = dce_pthread_cond_wait;
  libc->pthread_condattr_destroy_fn = dce_pthread_condattr_destroy;
  libc->pthread_condattr_init_fn = dce_pthread_condattr_init;

  libc->gethostbyname_fn = dce_gethostbyname;
  libc->gethostbyname2_fn = dce_gethostbyname2;
  libc->getaddrinfo_fn = dce_getaddrinfo;
  libc->freeaddrinfo_fn = dce_freeaddrinfo;
  libc->gai_strerror_fn = dce_gai_strerror;

  libc->getenv_fn = dce_getenv;
  libc->putenv_fn = dce_putenv;
  libc->setenv_fn = dce_setenv;
  libc->unsetenv_fn = dce_unsetenv;
  libc->clearenv_fn = dce_clearenv;

  libc->toupper_fn = toupper;
  libc->tolower_fn = tolower;

  libc->timerfd_create_fn = dce_timerfd_create;
  libc->timerfd_settime_fn = dce_timerfd_settime;
  libc->timerfd_gettime_fn = dce_timerfd_gettime;

  libc->if_nametoindex_fn = dce_if_nametoindex;
  libc->fork_fn = dce_fork;
}

#ifdef __cplusplus
}
#endif
