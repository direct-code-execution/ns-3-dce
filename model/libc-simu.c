#define _GNU_SOURCE 1
#include "libc-simu.h"
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <locale.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <ctype.h>
#include <time.h>
#include "sys/simu-socket.h"
#include "simu-unistd.h"
#include "simu-netdb.h"
#include "simu-pthread.h"
#include "simu-stdio.h"
#include "simu-stdarg.h"
#include "simu-errno.h"
#include "simu-fcntl.h"
#include "sys/simu-time.h"
#include "sys/simu-ioctl.h"
#include "simu-sched.h"
#include "simu-poll.h"
#include "simu-signal.h"
#include "simu-stdlib.h"
#include "simu-time.h"
#include "sys/simu-mman.h"
#include "sys/simu-stat.h"
#include "sys/simu-select.h"
#include "simu-semaphore.h"
#include "arpa/simu-inet.h"
#include "simu-cxa.h"
#include "simu-string.h"
#include "simu-global-variables.h"
#include "sys/simu-timerfd.h"
#include "simu-random.h"
#include "net/simu-if.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int (*__gxx_personality_v0) (int a, int b, 
				    unsigned c, 
				    struct _Unwind_Exception *d, 
				    struct _Unwind_Context *e);
extern int __xpg_strerror_r (int __errnum, char *__buf, size_t __buflen);


void libc_simu (struct Libc *libc)
{
  libc->global_variables_setup = simu_global_variables_setup;
  libc->strerror_fn = strerror;
  libc->gnu_strerror_r_fn = strerror_r;
  libc->xsi_strerror_r_fn = __xpg_strerror_r;
  libc->atexit_fn = simu_atexit;
  libc->__cxa_finalize_fn = simu__cxa_finalize;
  libc->__cxa_atexit_fn = simu__cxa_atexit;
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
  libc->socket_fn = simu_socket;
  libc->getsockname_fn = simu_getsockname;
  libc->getpeername_fn = simu_getpeername;
  libc->bind_fn = simu_bind;
  libc->read_fn = simu_read;
  libc->connect_fn = simu_connect;
  libc->write_fn = simu_write;
  libc->writev_fn = simu_writev;
  libc->memset_fn = memset;
  libc->memcpy_fn = memcpy;
  libc->memcmp_fn = memcmp;
  libc->memmove_fn = memmove;
  libc->strcpy_fn = strcpy;
  libc->strncpy_fn = strncpy;
  libc->strcmp_fn = strcmp;
  libc->strncmp_fn = strncmp;
  libc->strlen_fn = strlen;
  libc->strchr_fn = strchr;
  libc->strrchr_fn = strrchr;
  libc->strcasecmp_fn = strcasecmp;
  libc->strncasecmp_fn = strncasecmp;
  libc->strdup_fn = simu_strdup;
  libc->strndup_fn = simu_strndup;
  libc->sleep_fn = simu_sleep;
  libc->fopen_fn = simu_fopen;
  libc->fdopen_fn = simu_fdopen;
  libc->freopen_fn = simu_freopen;
  libc->fclose_fn = simu_fclose;
  libc->fcloseall_fn = simu_fcloseall;
  libc->fread_fn = simu_fread;
  libc->fwrite_fn = simu_fwrite;
  libc->fflush_fn = simu_fflush;
  libc->fseek_fn = simu_fseek;
  libc->ftell_fn = simu_ftell;
  libc->fgetpos_fn = simu_fgetpos;
  libc->fsetpos_fn = simu_fsetpos;
  libc->ferror_fn = simu_ferror;
  libc->feof_fn = simu_feof;
  libc->fileno_fn = simu_fileno;
  libc->clearerr_fn = simu_clearerr;
  libc->setvbuf_fn = simu_setvbuf;
  libc->rewind_fn = simu_rewind;
  libc->vfprintf_fn = simu_vfprintf;
  libc->vsprintf_fn = vsprintf;
  libc->vsnprintf_fn = vsnprintf;
  libc->fputc_fn = simu_fputc;
  libc->fputs_fn = simu_fputs;
  libc->fgetc_fn = simu_fgetc;
  libc->fgets_fn = simu_fgets;
  libc->ungetc_fn = simu_ungetc;

  libc->fcntl_fn = simu_fcntl;
  libc->nanosleep_fn = simu_nanosleep;

  libc->random_fn = simu_random;
  libc->rand_fn = simu_rand;
  libc->srandom_fn = simu_srandom;
  libc->srand_fn = simu_srand;

  libc->__errno_location_fn = simu_get_errno;
  libc->getopt_r_fn = simu_getopt_r;
  libc->getopt_long_r_fn = simu_getopt_long_r;
  libc->getpid_fn = simu_getpid;
  libc->getppid_fn = simu_getppid;
  libc->getuid_fn = simu_getuid;
  libc->geteuid_fn = simu_geteuid;
  libc->setuid_fn = simu_setuid;
  libc->setgid_fn = simu_setgid;
  libc->seteuid_fn = simu_seteuid;
  libc->setegid_fn = simu_setegid;
  libc->setreuid_fn = simu_setreuid;
  libc->setregid_fn = simu_setregid;
  libc->setresuid_fn = simu_setresuid;
  libc->setresgid_fn = simu_setresgid;
  libc->inet_aton_fn = inet_aton;
  libc->inet_ntoa_fn = inet_ntoa;
  libc->inet_ntop_fn = simu_inet_ntop;
  libc->inet_network_fn = inet_network;
  libc->inet_makeaddr_fn = inet_makeaddr;
  libc->inet_lnaof_fn = inet_lnaof;
  libc->inet_netof_fn = inet_netof;
  libc->inet_addr_fn = inet_addr;
  libc->mmap64_fn = simu_mmap64;
  libc->munmap_fn = simu_munmap;
  libc->xstat_fn = simu_xstat;
  libc->lxstat_fn = simu_lxstat;
  libc->fxstat_fn = simu_fxstat;
  libc->xstat64_fn = simu_xstat64;
  libc->lxstat64_fn = simu_lxstat64;
  libc->fxstat64_fn = simu_fxstat64;
  libc->dup_fn = simu_dup;
  libc->dup2_fn = simu_dup2;
  libc->open_fn = simu_open;
  libc->close_fn = simu_close;
  libc->unlink_fn = simu_unlink;
  libc->remove_fn = simu_remove;
  libc->mkdir_fn = simu_mkdir;
  libc->rmdir_fn = simu_rmdir;
  libc->setsockopt_fn = simu_setsockopt;
  libc->getsockopt_fn = simu_getsockopt;
  libc->select_fn = simu_select;
  libc->listen_fn = simu_listen;
  libc->accept_fn = simu_accept;
  libc->shutdown_fn = simu_shutdown;
  libc->calloc_fn = simu_calloc;
  libc->malloc_fn = simu_malloc;
  libc->free_fn = simu_free;
  libc->realloc_fn = simu_realloc;
  libc->gettimeofday_fn = simu_gettimeofday;
  libc->time_fn = simu_time;
  libc->isatty_fn = simu_isatty;
  libc->send_fn = simu_send;
  libc->sendto_fn = simu_sendto;
  libc->sendmsg_fn = simu_sendmsg;
  libc->recv_fn = simu_recv;
  libc->recvfrom_fn = simu_recvfrom;
  libc->recvmsg_fn = simu_recvmsg;
  libc->ioctl_fn = simu_ioctl;
  libc->sched_yield_fn = simu_sched_yield;
  libc->poll_fn = simu_poll;
  libc->__cmsg_nxthdr_fn = __cmsg_nxthdr;
  libc->atoi_fn = atoi;
  libc->atol_fn = atol;
  libc->atoll_fn = atoll;
  libc->atof_fn = atof;
  libc->setitimer_fn = simu_setitimer;
  libc->getitimer_fn = simu_getitimer;
  libc->exit_fn = simu_exit;
  libc->signal_fn = simu_signal;
  libc->sigaction_fn = simu_sigaction;
  libc->sigemptyset_fn = sigemptyset;
  libc->sigfillset_fn = sigfillset;
  libc->sigaddset_fn = sigaddset;
  libc->sigdelset_fn = sigdelset;
  libc->sigismember_fn = sigismember;
  libc->strtol_fn = simu_strtol;
  libc->strtoll_fn = simu_strtoll;
  libc->strtoul_fn = simu_strtoul;
  libc->strtoull_fn = simu_strtoull;
  libc->strtod_fn = simu_strtod;
  libc->getcwd_fn = simu_getcwd;
  libc->getwd_fn = simu_getwd;
  libc->get_current_dir_name_fn = simu_get_current_dir_name;
  libc->chdir_fn = simu_chdir;
  libc->fchdir_fn = simu_fchdir;
  libc->localtime_fn = localtime;
  libc->strftime_fn = strftime;
  libc->__ctype_b_loc_fn = __ctype_b_loc;
  libc->gmtime_r_fn = gmtime_r;

  libc->pthread_create_fn = simu_pthread_create;
  libc->pthread_exit_fn = simu_pthread_exit;
  libc->pthread_self_fn = simu_pthread_self;
  libc->pthread_once_fn = simu_pthread_once;
  libc->pthread_getspecific_fn = simu_pthread_getspecific;
  libc->pthread_setspecific_fn = simu_pthread_setspecific;
  libc->pthread_key_create_fn = simu_pthread_key_create;
  libc->pthread_key_delete_fn = simu_pthread_key_delete;
  libc->pthread_mutex_destroy_fn = simu_pthread_mutex_destroy;
  libc->pthread_mutex_init_fn = simu_pthread_mutex_init;
  libc->pthread_mutex_lock_fn = simu_pthread_mutex_lock;
  libc->pthread_mutex_unlock_fn = simu_pthread_mutex_unlock;
  libc->pthread_mutex_trylock_fn = simu_pthread_mutex_trylock;
  libc->pthread_mutexattr_init_fn = simu_pthread_mutexattr_init;
  libc->pthread_mutexattr_destroy_fn = simu_pthread_mutexattr_destroy;
  libc->pthread_mutexattr_settype_fn = simu_pthread_mutexattr_settype;
  libc->pthread_cancel_fn = simu_pthread_cancel;
  libc->pthread_kill_fn = simu_pthread_kill;
  libc->pthread_join_fn = simu_pthread_join;
  libc->pthread_detach_fn = simu_pthread_detach;
  libc->sem_init_fn = simu_sem_init;
  libc->sem_destroy_fn = simu_sem_destroy;
  libc->sem_post_fn = simu_sem_post;
  libc->sem_wait_fn = simu_sem_wait;
  libc->sem_timedwait_fn = simu_sem_timedwait;
  libc->sem_trywait_fn = simu_sem_trywait;
  libc->sem_getvalue_fn = simu_sem_getvalue;
  libc->pthread_cond_destroy_fn = simu_pthread_cond_destroy;
  libc->pthread_cond_init_fn = simu_pthread_cond_init;
  libc->pthread_cond_broadcast_fn = simu_pthread_cond_broadcast;
  libc->pthread_cond_signal_fn = simu_pthread_cond_signal;
  libc->pthread_cond_timedwait_fn = simu_pthread_cond_timedwait;
  libc->pthread_cond_wait_fn = simu_pthread_cond_wait;
  libc->pthread_condattr_destroy_fn = simu_pthread_condattr_destroy;
  libc->pthread_condattr_init_fn = simu_pthread_condattr_init;

  libc->gethostbyname_fn = simu_gethostbyname;
  libc->gethostbyname2_fn = simu_gethostbyname2;
  libc->getaddrinfo_fn = simu_getaddrinfo;
  libc->freeaddrinfo_fn = simu_freeaddrinfo;
  libc->gai_strerror_fn = simu_gai_strerror;

  libc->getenv_fn = simu_getenv;
  libc->putenv_fn = simu_putenv;
  libc->setenv_fn = simu_setenv;
  libc->unsetenv_fn = simu_unsetenv;
  libc->clearenv_fn = simu_clearenv;

  libc->toupper_fn = toupper;
  libc->tolower_fn = tolower;

  libc->timerfd_create_fn = simu_timerfd_create;
  libc->timerfd_settime_fn = simu_timerfd_settime;
  libc->timerfd_gettime_fn = simu_timerfd_gettime;

  libc->if_nametoindex_fn = simu_if_nametoindex;
}

#ifdef __cplusplus
}
#endif
