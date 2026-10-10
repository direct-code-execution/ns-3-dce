/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef DCE_FUTEX_H
#define DCE_FUTEX_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * syscall(2) of a DCE application. The futex(2) system call, which GLib,
 * Rust's standard library and other lock implementations use directly, is
 * emulated with the DCE scheduler: a native futex would block the whole
 * simulator. Other system calls are passed to the host.
 */
long dce_syscall (long number, ...);

#ifdef __cplusplus
}
#endif

#endif /* DCE_FUTEX_H */
