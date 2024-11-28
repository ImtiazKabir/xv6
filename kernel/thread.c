#include "common/riscv.h"
#include "file.h"
#include "fs.h"
#include "kernel/proc.h"
#include "string.h"
#include "syscall.h"
#include "vm.h"

#define FAKE_RETURN_ADDRESS 0xffffffff

uint64 sys_thread_create(void) {
  auto uint64 stack = 0u;

  int i, pid;
  struct proc *np;
  struct proc *p = myproc();

  // Allocate process.
  if ((np = allocproc()) == 0) {
    return 0;
  }

  if (uvmmirror(p->pagetable, np->pagetable, p->sz) < 0) {
    freeproc(np);
    release(&np->lock);
    return 0;
  }
  np->sz = p->sz;

  // copy saved user registers.
  *(np->trapframe) = *(p->trapframe);

  // increment reference counts on open file descriptors.
  for (i = 0; i < NOFILE; i++)
    if (p->ofile[i])
      np->ofile[i] = filedup(p->ofile[i]);
  np->cwd = idup(p->cwd);

  safestrcpy(np->name, p->name, sizeof(p->name));

  pid = np->pid;
  release(&np->lock);

  acquire(&wait_lock);
  np->parent = p;
  release(&wait_lock);

  acquire(&np->lock);
  np->state = RUNNABLE;
  argaddr(0, &(np->trapframe->epc));
  argaddr(1, &(np->trapframe->a0));
  argaddr(2, &stack);
  np->trapframe->sp = stack + PGSIZE;
  np->trapframe->ra = FAKE_RETURN_ADDRESS;
  np->is_thread = 1;
  release(&np->lock);

  return pid;
}

// Wait for a thread to exit
// Return -1 if this process has no children.
uint64 sys_thread_join(void) {
  register int const tid = (int)argraw(0);
  struct proc *pp;
  int havekids;
  struct proc *p = myproc();

  acquire(&wait_lock);

  for (;;) {
    // Scan through table looking for exited children.
    havekids = 0;
    for (pp = proc; pp < &proc[NPROC]; pp++) {
      if (pp->parent == p && pp->is_thread == 1) {
        // make sure the child isn't still in exit() or swtch().
        acquire(&pp->lock);

        havekids = 1;
        if (pp->state == ZOMBIE && pp->pid == tid) {
          // Found thread.
          freeproc(pp);
          release(&pp->lock);
          release(&wait_lock);
          return (uint64)tid;
        }
        release(&pp->lock);
      }
    }

    // No point waiting if we don't have any children.
    if (!havekids || killed(p)) {
      release(&wait_lock);
      return (uint64)-1;
    }

    // Wait for a child to exit.
    sleep(p, &wait_lock); // DOC: wait-sleep
  }
}

uint64 sys_thread_exit(void) {
  exit(0);
  return 0;
}
