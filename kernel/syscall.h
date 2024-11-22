#ifndef KERNEL_SYSCALL_H_
#define KERNEL_SYSCALL_H_

#include "common/types.h"
#include "common/syscall.h"

// syscall.c
void argint(int n, int *ip);
int argstr(int n, char *buf, int max);
uint64 argraw(int n);
void argaddr(int n, uint64 *ip);
int fetchstr(uint64 addr, char *buf, int max);
int fetchaddr(uint64 addr, uint64 *ip);
void syscall(void);

// Prototypes for the functions that handle system calls.
uint64 sys_fork(void);
uint64 sys_exit(void);
uint64 sys_wait(void);
uint64 sys_pipe(void);
uint64 sys_read(void);
uint64 sys_kill(void);
uint64 sys_exec(void);
uint64 sys_fstat(void);
uint64 sys_chdir(void);
uint64 sys_dup(void);
uint64 sys_getpid(void);
uint64 sys_sbrk(void);
uint64 sys_sleep(void);
uint64 sys_uptime(void);
uint64 sys_open(void);
uint64 sys_write(void);
uint64 sys_mknod(void);
uint64 sys_unlink(void);
uint64 sys_link(void);
uint64 sys_mkdir(void);
uint64 sys_close(void);
uint64 sys_trace(void);
uint64 sys_info(void);
uint64 sys_getlast(void);
uint64 sys_setlast(void);
uint64 sys_seed(void);
uint64 sys_rand(void);
uint64 sys_thread_create(void);
uint64 sys_thread_join(void);
uint64 sys_thread_exit(void);

extern uint64 (*syscalls[])(void);

#endif // !KERNEL_SYSCALL_H_
