#include "common/memlayout.h"
#include "common/param.h"
#include "common/procinfo.h"
#include "common/pstat.h"
#include "common/types.h"

#include "proc.h"
#include "spinlock.h"
#include "string.h"
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

uint64 sys_fork(void) { return fork(); }

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
  auto uint64 dst = 0;
  auto struct procInfo info = {.activeProcess = 0,
                               .totalProcess = NPROC,
                               .memsize = 0,
                               .totalMemSize = PHYSTOP - KERNBASE};
  register struct proc *p = 0;

  for (p = proc; p < &proc[NPROC]; p += 1) {
    acquire(&p->lock);
    if ((p->state == RUNNING) || (p->state == RUNNABLE)) {
      info.activeProcess += 1;
      info.memsize += p->sz;
    }
    release(&p->lock);
  }

  argaddr(0, &dst);
  (void)copyout(myproc()->pagetable, (uint64)dst, (void *)&info, sizeof(info));

  return 0u;
}

uint64 sys_getpinfo(void) {
  auto uint64 dst = 0;
  auto struct pstat stat = {0};

  fill_pinfo(&stat);

  argaddr(0, &dst);
  if (dst == 0) {
    return (uint64)-1;
  }
  (void)copyout(myproc()->pagetable, (uint64)dst, (void *)&stat, sizeof(stat));

  return 0u;
}

uint64 sys_settickets(void) {
  register struct proc *const p = myproc();
  register uint const tickets = (uint)argraw(0);
  register uint64 ret = 0;
  acquire(&p->lock);
  if (tickets > 0) {
    p->tickets_original = tickets;
  } else {
    p->tickets_original = DEFAULT_TICKETS;
    ret = (uint64)-1;
  }
  p->tickets_current = p->tickets_original;
  release(&p->lock);
  return ret;
}

