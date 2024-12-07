#include "thread.h"

#include "common/memlayout.h"
#include "common/riscv.h"

#include "file.h"
#include "fs.h"
#include "kernel/proc.h"
#include "string.h"
#include "syscall.h"
#include "tvm.h"
#include "vm.h"

#define FAKE_RETURN_ADDRESS 0xffffffff

// Create a new thread, copying the parent.
// Sets up child kernel stack to return as if from thread_create() system call.
extern int thread_create(register uint64 const start, register uint64 const arg,
                         register uint64 const stack) {
  /* Copied from fork() */
  /* CHANGE: 1) calling tvmcopy instead of uvmcopy, 2) initializing thread */
  int i, pid;
  struct proc *np;
  struct proc *p = myproc();

  // Allocate process.
  if ((np = allocproc()) == 0) {
    return 0;
  }

  if (tvmcopy(p->pagetable, np->pagetable, p->sz) < 0) {
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
  np->trapframe->epc = start;
  np->trapframe->a0 = arg;
  np->trapframe->sp = stack + PGSIZE;
  np->trapframe->ra = FAKE_RETURN_ADDRESS;
  np->is_thread = 1;
  np->memlock = p->memlock;
  np->state = RUNNABLE;
  release(&np->lock);

  return pid;
}

// Wait for a thread to exit
// Return -1 if this process has no children.
extern int thread_join(register int const tid) {
  /* Copied from wait() */
  /* CHANGE: only looking for a perticular thread, also not passing the ret val
   */
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
          return tid;
        }
        release(&pp->lock);
      }
    }

    // No point waiting if we don't have any children.
    if (!havekids || killed(p)) {
      release(&wait_lock);
      return -1;
    }

    // Wait for a child to exit.
    sleep(p, &wait_lock); // DOC: wait-sleep
  }
}

extern void thread_exit(void) { exit(0); }

// Free a process's page table, and free the
// physical memory it refers to.
extern void thread_freepagetable(register pagetable_t const pagetable,
                                 register uint64 const sz) {
  uvmunmap(pagetable, TRAMPOLINE, 1, 0);
  uvmunmap(pagetable, TRAPFRAME, 1, 0);
  tvmfree(pagetable, sz);
}
