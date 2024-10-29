#include "syscall.h"
#include "proc.h"
#include "common/util.h"
#include "printf.h"

// An array mapping syscall numbers from syscall.h
// to the tracable names
// clang-format off
static char const *const syscall_name[] = {
[SYS_fork]    = "fork",
[SYS_exit]    = "exit",
[SYS_wait]    = "wait",
[SYS_pipe]    = "pipe",
[SYS_read]    = "read",
[SYS_kill]    = "kill",
[SYS_exec]    = "exec",
[SYS_fstat]   = "fstat",
[SYS_chdir]   = "chdir",
[SYS_dup]     = "dup",
[SYS_getpid]  = "getpid",
[SYS_sbrk]    = "sbrk",
[SYS_sleep]   = "sleep",
[SYS_uptime]  = "uptime",
[SYS_open]    = "open",
[SYS_write]   = "write",
[SYS_mknod]   = "mknod",
[SYS_unlink]  = "unlink",
[SYS_link]    = "link",
[SYS_mkdir]   = "mkdir",
[SYS_close]   = "close",
[SYS_trace]   = "trace",
[SYS_info]    = "info",
[SYS_getlast] = "getlast",
[SYS_setlast] = "setlast",
[SYS_seed] = "seed",
[SYS_rand] = "rand"
};
// clang-format on

void trace(struct proc *p) {
  int num = p->trapframe->a7;
  enum { MAX_BUF = 127u };
  char buf[MAX_BUF] = {0};
  acquire(&p->lock);
  printf("pid: %d, syscall: %s, args: (", p->pid, syscall_name[num]);
  release(&p->lock);
  switch (num) {
    case SYS_fork:
      break;
    case SYS_exit:
      printf("%d)\n, __noreturn__", (int)argraw(0));
      break;
    case SYS_wait:
      printf("%p", (void *)argraw(0));
      break;
    case SYS_pipe:
      printf("%p", (void *)argraw(0));
      break;
    case SYS_read:
      printf("%d, %p, %d", (int)argraw(0), (void *)argraw(1), (int)argraw(2));
      break;
    case SYS_kill:
      printf("%d", (int)argraw(0));
      break;
    case SYS_exec:
      argstr(0, buf, MAX_BUF);
      printf("%s, %p", buf, (void *)argraw(1));
      break;
    case SYS_fstat:
      printf("%d, %p", (int)argraw(0), (void *)argraw(1));
      break;
    case SYS_chdir:
      argstr(0, buf, MAX_BUF);
      printf("%s", buf);
      break;
    case SYS_dup:
      printf("%d", (int)argraw(0));
      break;
    case SYS_getpid:
      break;
    case SYS_sbrk:
      printf("%d", (int)argraw(0));
      break;
    case SYS_sleep:
      printf("%d", (int)argraw(0));
      break;
    case SYS_uptime:
      break;
    case SYS_open:
      argstr(0, buf, MAX_BUF);
      printf("%s, %d", buf, (int)argraw(1));
      break;
    case SYS_write:
      printf("%d, %p, %d", (int)argraw(0), (void *)argraw(1), (int)argraw(2));
      break;
    case SYS_mknod:
      argstr(0, buf, MAX_BUF);
      printf("%s, %d, %d", buf, (short)argraw(1), (short)argraw(2));
      break;
    case SYS_unlink:
      argstr(0, buf, MAX_BUF);
      printf("%s", buf);
      break;
    case SYS_link:
      argstr(0, buf, MAX_BUF);
      printf("%s, ", buf);
      argstr(0, buf, MAX_BUF);
      printf("%s", buf);
      break;
    case SYS_mkdir:
      argstr(0, buf, MAX_BUF);
      printf("%s", buf);
      break;
    case SYS_close:
      printf("%d", (int)argraw(0));
      break;
    case SYS_trace:
      printf("%d", (int)argraw(0));
      break;
    case SYS_info:
      printf("%p", (void *)argraw(0));
      break;
    case SYS_getlast:
      argstr(0, buf, MAX_BUF);
      printf("%s, ", buf);
      printf("%d", (int)argraw(1));
      break;
    case SYS_setlast:
      argstr(0, buf, MAX_BUF);
      printf("%s, ", buf);
      printf("%d", (int)argraw(1));
      break;
    case SYS_seed:
      printf("%d", (int)argraw(0));
      break;
    case SYS_rand:
      break;
    default:
      printf("Adding a new syscall? Try making it tracable\n");
  }
}

uint64 sys_trace(void) {
  auto int trace_id;
  argint(0, &trace_id);
  if ((trace_id < 1) || (trace_id >= (int)NELEM(syscall_name))) {
    return (uint64)(-1);
  }
  myproc()->trace_id = trace_id;
  return 0u;
}
