/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef DCE_EPOLL_H
#define DCE_EPOLL_H

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <signal.h>

#ifdef __cplusplus
extern "C" {
#endif

int dce_epoll_create (int size);
int dce_epoll_create1 (int flags);
int dce_epoll_ctl (int epfd, int op, int fd, struct epoll_event *event);
int dce_epoll_wait (int epfd, struct epoll_event *events, int maxevents, int timeout);
int dce_epoll_pwait (int epfd, struct epoll_event *events, int maxevents, int timeout,
                     const sigset_t *sigmask);
int dce_eventfd (unsigned int initval, int flags);
int dce_eventfd_read (int fd, eventfd_t *value);
int dce_eventfd_write (int fd, eventfd_t value);

#ifdef __cplusplus
}
#endif

#endif /* DCE_EPOLL_H */
