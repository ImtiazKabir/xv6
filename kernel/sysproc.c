#include "common/memlayout.h"
#include "common/param.h"
#include "common/procinfo.h"
#include "common/types.h"

#include "proc.h"
#include "syscall.h"
#include "trap.h"
#include "vm.h"

uint64 sys_exit(void) {
  int n;
  argint(0, &n);
  myproc()->trace_id = 0; /* unset trace_id at exit */
  exit(n);
  return 0; // not reached
}

uint64 sys_getpid(void) { return myproc()->pid; }

uint64 sys_fork(void) { 
  return fork()->pid;
}

uint64 sys_wait(void) {
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64 sys_sbrk(void) {
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if (growproc(n) < 0)
    return -1;
  return addr;
}

uint64 sys_sleep(void) {
  int n;
  uint ticks0;

  argint(0, &n);
  if (n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n) {
    if (killed(myproc())) {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64 sys_kill(void) {
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64 sys_uptime(void) {
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64 sys_info(void) {
  auto void *dst = 0;
  auto struct procInfo info = {.activeProcess = 0,
                               .totalProcess = NPROC,
                               .memsize = 0,
                               .totalMemSize = PHYSTOP - KERNBASE};
  register struct proc *p = 0;
  ;

  for (p = proc; p < &proc[NPROC]; p += 1) {
    acquire(&p->lock);
    if ((p->state == RUNNING) || (p->state == RUNNABLE)) {
      info.activeProcess += 1;
      info.memsize += p->sz;
    }
    release(&p->lock);
  }

  argaddr(0, (void *)&dst);
  (void)copyout(myproc()->pagetable, (uint64)dst, (void *)&info, sizeof(info));

  return 0u;
}
