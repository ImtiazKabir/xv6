#include "common/riscv.h"
#include "kernel/proc.h"
#include "syscall.h"
#include "printf.h"

#define FAKE_RETURN_ADDRESS 0xffffffff

uint64 sys_thread_create(void) {
  register struct proc *const np = fork(0);
  auto uint64 stack = 0u;

  argaddr(0, &(np->trapframe->epc));
  argaddr(1, &(np->trapframe->a0));

  argaddr(2, &stack);
  np->trapframe->sp = stack + PGSIZE;

  np->trapframe->ra = FAKE_RETURN_ADDRESS;
  np->is_thread = 1;

  return (uint64) np->pid;
}

// Wait for a thread to exit
// Return -1 if this process has no children.
uint64 sys_thread_join(void) {
  register int const tid = (int) argraw(0);
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

