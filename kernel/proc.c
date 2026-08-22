#include "proc.h"
#include "common/memlayout.h"
#include "common/param.h"
#include "common/pstat.h"
#include "common/riscv.h"
#include "common/types.h"
#include "common/ansi.h"
#include "common/util.h"
#include "file.h"
#include "fs.h"
#include "kalloc.h"
#include "log.h"
#include "printf.h"
#include "rand.h"
#include "riscv.h"
#include "spinlock.h"
#include "string.h"
#include "swtch.h"
#include "syscall.h"
#include "trap.h"
#include "vm.h"

struct cpu cpus[NCPU];

struct proc proc[NPROC] = {0};

struct proc *initproc = 0;

int nextpid = 1;
struct spinlock pid_lock;

extern void forkret(void);
static void freeproc(struct proc *p);

extern char trampoline[]; // trampoline.S

// helps ensure that wakeups of wait()ing
// parents are not lost. helps obey the
// memory model when using p->parent.
// must be acquired before any p->lock.
struct spinlock wait_lock;

// Allocate a page for each process's kernel stack.
// Map it high in memory, followed by an invalid
// guard page.
void proc_mapstacks(pagetable_t kpgtbl) {
  struct proc *p;

  for (p = proc; p < &proc[NPROC]; p++) {
    char *pa = kalloc();
    if (pa == 0)
      panic("kalloc");
    uint64 va = KSTACK((int)(p - proc));
    kvmmap(kpgtbl, va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
  }
}

// initialize the proc table.
void procinit(void) {
  struct proc *p;

  initlock(&pid_lock, "nextpid");
  initlock(&wait_lock, "wait_lock");
  for (p = proc; p < &proc[NPROC]; p++) {
    initlock(&p->lock, "proc");
    p->state = UNUSED;
    p->kstack = KSTACK((int)(p - proc));
  }
}

// Must be called with interrupts disabled,
// to prevent race with process being moved
// to a different CPU.
int cpuid() {
  int id = r_tp();
  return id;
}

// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct cpu *mycpu(void) {
  int id = cpuid();
  struct cpu *c = &cpus[id];
  return c;
}

// Return the current struct proc *, or zero if none.
struct proc *myproc(void) {
  push_off();
  struct cpu *c = mycpu();
  struct proc *p = c->proc;
  pop_off();
  return p;
}

int allocpid() {
  int pid;

  acquire(&pid_lock);
  pid = nextpid;
  nextpid = nextpid + 1;
  release(&pid_lock);

  return pid;
}

// Look in the process table for an UNUSED proc.
// If found, initialize state required to run in the kernel,
// and return with p->lock held.
// If there are no free procs, or a memory allocation fails, return 0.
static struct proc *allocproc(void) {
  struct proc *p;

  for (p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if (p->state == UNUSED) {
      goto found;
    } else {
      release(&p->lock);
    }
  }
  return 0;

found:
  p->pid = allocpid();
  p->state = USED;

  // Allocate a trapframe page.
  if ((p->trapframe = (struct trapframe *)kalloc()) == 0) {
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // Allocate a cshared page
  if ((p->pshared = (struct trapframe *)kalloc()) == 0) {
    // kfree(p->trapframe);
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // Allocate a cshared page
  if ((p->cshared = (struct trapframe *)kalloc()) == 0) {
    // kfree(p->pshared);
    // kfree(p->trapframe);
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // An empty user page table.
  p->pagetable = proc_pagetable(p);
  if (p->pagetable == 0) {
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // Set up new context to start executing at forkret,
  // which returns to user space.
  memset(&p->context, 0, sizeof(p->context));
  p->context.ra = (uint64)forkret;
  p->context.sp = p->kstack + PGSIZE;

  p->queue_index = DEFAULT_QUEUE;
  p->tickets_original = DEFAULT_TICKETS;
  p->tickets_current = p->tickets_original;

  return p;
}

// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.
static void freeproc(struct proc *p) {
  if (p->trapframe)
    kfree((void *)p->trapframe);
  p->trapframe = 0;
  if (p->pshared)
    kfree(p->pshared);
  if (p->cshared)
    kfree(p->cshared);
  if (p->pagetable)
    proc_freepagetable(p->pagetable, p->sz);
  p->pagetable = 0;
  p->sz = 0;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  p->state = UNUSED;
  p->trace_id = 0;
}

// Create a user page table for a given process, with no user memory,
// but with trampoline, trapframe and shared pages.
pagetable_t proc_pagetable(struct proc *p) {
  pagetable_t pagetable;

  // An empty page table.
  pagetable = uvmcreate();
  if (pagetable == 0)
    return 0;

  // map the trampoline code (for system call return)
  // at the highest user virtual address.
  // only the supervisor uses it, on the way
  // to/from user space, so not PTE_U.
  if (mappages(pagetable, TRAMPOLINE, PGSIZE, (uint64)trampoline,
               PTE_R | PTE_X) < 0) {
    uvmfree(pagetable, 0);
    return 0;
  }

  // map the trapframe page just below the trampoline page, for
  // trampoline.S.
  if (mappages(pagetable, TRAPFRAME, PGSIZE, (uint64)(p->trapframe),
               PTE_R | PTE_W) < 0) {
    uvmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmfree(pagetable, 0);
    return 0;
  }

  if (mappages(pagetable, PSHARED, PGSIZE, (uint64)(p->pshared),
               PTE_R | PTE_W | PTE_U) < 0) {
    uvmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmfree(pagetable, 0);
    return 0;
  }

  // map the cshared page
  if (mappages(pagetable, CSHARED, PGSIZE, (uint64)(p->cshared),
               PTE_R | PTE_W | PTE_U) < 0) {
    if (initproc == 0) {
      uvmunmap(pagetable, PSHARED, 1, 0);
    }
    uvmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmfree(pagetable, 0);
    return 0;
  }

  return pagetable;
}

// Free a process's page table, and free the
// physical memory it refers to.
void proc_freepagetable(pagetable_t pagetable, uint64 sz) {
  uvmunmap(pagetable, TRAMPOLINE, 1, 0);
  uvmunmap(pagetable, TRAPFRAME, 1, 0);
  uvmunmap(pagetable, PSHARED, 1, 0);
  uvmunmap(pagetable, CSHARED, 1, 0);
  uvmfree(pagetable, sz);
}

// a user program that calls exec("/init")
// assembled from ../user/initcode.S
// od -t xC ../user/initcode
uchar initcode[] = {0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x45, 0x02, 0x97,
                    0x05, 0x00, 0x00, 0x93, 0x85, 0x35, 0x02, 0x93, 0x08,
                    0x70, 0x00, 0x73, 0x00, 0x00, 0x00, 0x93, 0x08, 0x20,
                    0x00, 0x73, 0x00, 0x00, 0x00, 0xef, 0xf0, 0x9f, 0xff,
                    0x2f, 0x69, 0x6e, 0x69, 0x74, 0x00, 0x00, 0x24, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// Set up first user process.
void userinit(void) {
  struct proc *p;

  p = allocproc();
  initproc = p;

  // allocate one user page and copy initcode's instructions
  // and data into it.
  uvmfirst(p->pagetable, initcode, sizeof(initcode));
  p->sz = PGSIZE;

  // prepare for the very first "return" from kernel to user.
  p->trapframe->epc = 0;     // user program counter
  p->trapframe->sp = PGSIZE; // user stack pointer

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->state = RUNNABLE;

  release(&p->lock);
}

// Grow or shrink user memory by n bytes.
// Return 0 on success, -1 on failure.
int growproc(int n) {
  uint64 sz;
  struct proc *p = myproc();

  sz = p->sz;
  if (n > 0) {
    if ((sz = uvmalloc(p->pagetable, sz, sz + n, PTE_W)) == 0) {
      return -1;
    }
  } else if (n < 0) {
    sz = uvmdealloc(p->pagetable, sz, sz + n);
  }
  p->sz = sz;
  return 0;
}

// Create a new process, copying the parent.
// Sets up child kernel stack to return as if from fork() system call.
int fork(void) {
  int i, pid;
  struct proc *np;
  struct proc *p = myproc();

  // Allocate process.
  if ((np = allocproc()) == 0) {
    return -1;
  }

  // Copy user memory from parent to child.
  if (uvmcopy(p->pagetable, np->pagetable, p->sz) < 0) {
    freeproc(np);
    release(&np->lock);
    return -1;
  }
  np->sz = p->sz;

  // copy saved user registers.
  *(np->trapframe) = *(p->trapframe);

  // unmap pshared page, because it was mapped to the same physical pshared
  // page of the parent process
  uvmunmap(np->pagetable, PSHARED, 1, 0);
  // map pshared page for the child process to the cshared page of the parent
  // process
  if (mappages(np->pagetable, PSHARED, PGSIZE, (uint64)(p->cshared),
               PTE_R | PTE_W | PTE_U) < 0) {
    uvmunmap(np->pagetable, TRAMPOLINE, 1, 0);
    uvmfree(np->pagetable, 0);
    return 0;
  }

  // unmap cshared page, because it was mapped to the same physical cshared page
  // of the parent process
  uvmunmap(np->pagetable, CSHARED, 1, 0);
  // map new cshared page for the child process
  if (mappages(np->pagetable, CSHARED, PGSIZE, (uint64)(np->cshared),
               PTE_R | PTE_W | PTE_U) < 0) {
    uvmunmap(np->pagetable, TRAMPOLINE, 1, 0);
    uvmunmap(np->pagetable, PSHARED, 1, 0);
    uvmfree(np->pagetable, 0);
    return 0;
  }

  // Cause fork to return 0 in the child.
  np->trapframe->a0 = 0;

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
  release(&np->lock);

  /* child current tickets should be equal to parent's original ticket */
  /* A dying mother does not bear a dying child */
  np->tickets_original = p->tickets_original;
  np->tickets_current = np->tickets_original;

  return pid;
}

// Pass p's abandoned children to init.
// Caller must hold wait_lock.
void reparent(struct proc *p) {
  struct proc *pp;

  for (pp = proc; pp < &proc[NPROC]; pp++) {
    if (pp->parent == p) {
      pp->parent = initproc;
      wakeup(initproc);
    }
  }
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait().
void exit(int status) {
  struct proc *p = myproc();

  if (p == initproc)
    panic("init exiting");

  // Close all open files.
  for (int fd = 0; fd < NOFILE; fd++) {
    if (p->ofile[fd]) {
      struct file *f = p->ofile[fd];
      fileclose(f);
      p->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(p->cwd);
  end_op();
  p->cwd = 0;

  acquire(&wait_lock);

  // Give any children to init.
  reparent(p);

  // Parent might be sleeping in wait().
  wakeup(p->parent);

  acquire(&p->lock);

  p->xstate = status;
  p->state = ZOMBIE;

  release(&wait_lock);

  // Jump into the scheduler, never to return.
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int wait(uint64 addr) {
  struct proc *pp;
  int havekids, pid;
  struct proc *p = myproc();

  acquire(&wait_lock);

  for (;;) {
    // Scan through table looking for exited children.
    havekids = 0;
    for (pp = proc; pp < &proc[NPROC]; pp++) {
      if (pp->parent == p) {
        // make sure the child isn't still in exit() or swtch().
        acquire(&pp->lock);

        havekids = 1;
        if (pp->state == ZOMBIE) {
          // Found one.
          pid = pp->pid;
          if (addr != 0 && copyout(p->pagetable, addr, (char *)&pp->xstate,
                                   sizeof(pp->xstate)) < 0) {
            release(&pp->lock);
            release(&wait_lock);
            return -1;
          }
          freeproc(pp);
          release(&pp->lock);
          release(&wait_lock);
          return pid;
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

uint run(register struct proc *const p, register uint const time) {
  register uint t = 0;
  while ((p->state == RUNNABLE) && (p->running_time < time)) {
    // Switch to chosen process.  It is the process's job
    // to release its lock and then reacquire it
    // before jumping back to us.
    p->state = RUNNING;
    mycpu()->proc = p;
    swtch(&mycpu()->context, &p->context);
    mycpu()->proc = 0;
    p->running_time += 1;
    p->queue_ticks[p->queue_index] += 1;
    acquire(&tickslock);
    p->last_sched_time = ticks;
    release(&tickslock);
  }

  t = p->running_time;

  p->running_time = 0;
  p->times_scheduled += 1;

  return t;
}

int lottery_sched(void) {
  register struct proc *p = 0;
  auto uint index[NPROC] = {0};
  auto uint ticket[NPROC] = {0};
  register uint i = 0;
  register uint len = 0;
  register uint winner = 0;

  for (p = proc, i = 0; p < &proc[NPROC]; p += 1, i += 1) {
    acquire(&p->lock);
    if (p->state == RUNNABLE && p->queue_index == LOTTERY_QUEUE &&
        p->tickets_current > 0) {
      len += 1;
      index[len - 1] = i;
      ticket[len - 1] = p->tickets_current;
    }
    release(&p->lock);
  }

  if (len == 0) {
    return 1;
  }

  winner = index[choose((uint64 *)ticket, len)];
  acquire(&proc[winner].lock);

#ifdef DEBUG
  printf(ANSI_FG_YELLOW "LOTTERY:" ANSI_RESET " Process %d (%s) won in queue %d with tickets %u\n", proc[winner].pid, proc[winner].name, LOTTERY_QUEUE, proc[winner].tickets_current);
#endif /* ifdef DEBUG */

  {
    register int const rtime = run(&proc[winner], TIME_LIMIT_0);

#ifdef DEBUG
    printf(ANSI_FG_MAGENTA "INFO:" ANSI_RESET " Process %d (%s) has spent %u ticks in queue %d\n", proc[winner].pid, proc[winner].name, rtime, LOTTERY_QUEUE);
#endif /* ifdef DEBUG */

    if (rtime == TIME_LIMIT_0) {

#ifdef DEBUG
      printf(ANSI_FG_RED "DEMO:" ANSI_RESET " Process %d (%s) ran for %u timer ticks, demoted to queue %d\n", proc[winner].pid, proc[winner].name, rtime, ROUND_ROBIN_QUEUE);
#endif /* ifdef DEBUG */

      proc[winner].queue_index = ROUND_ROBIN_QUEUE;
    }
  }

  proc[winner].tickets_current -= 1;
  if (proc[winner].tickets_current == 0) {
#ifdef DEBUG
        printf(ANSI_FG_RED "DEMO:" ANSI_RESET " Process %d (%s) exhausted all its tickets, demoted to queue %d\n", proc[winner].pid, proc[winner].name, ROUND_ROBIN_QUEUE);
#endif /* ifdef DEBUG */
    proc[winner].queue_index = ROUND_ROBIN_QUEUE;
  }
  release(&proc[winner].lock);

  return 0;
}

void round_robin_sched(void) {
  struct proc *p = 0;
  for (p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);

    if (p->queue_index != ROUND_ROBIN_QUEUE) {
      release(&p->lock);
      continue;
    }

    if (p->state == RUNNABLE) {
      register uint const rtime = run(p, TIME_LIMIT_1);
#ifdef DEBUG
        printf(ANSI_FG_MAGENTA "INFO:" ANSI_RESET " Process %d (%s) has spent %u ticks in queue %d\n", p->pid, p->name, rtime, ROUND_ROBIN_QUEUE);
#endif /* ifdef DEBUG */
      if (rtime < TIME_LIMIT_1 && p->tickets_current > 0) {
#ifdef DEBUG
        printf(ANSI_FG_GREEN "PROMO:" ANSI_RESET " Process %d (%s) ran for %u timer ticks, promoted to queue %d\n", p->pid, p->name, rtime, LOTTERY_QUEUE);
#endif /* ifdef DEBUG */
        p->queue_index = LOTTERY_QUEUE;
      }
    }

    release(&p->lock);
  }
}

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run.
//  - swtch to start running that process.
//  - eventually that process transfers control
//    via swtch back to the scheduler.
void scheduler(void) {
  mycpu()->proc = 0;
  for (;;) {
    intr_on();
    while (lottery_sched() == 0) {
    }
    round_robin_sched();
  }
}

// Switch to scheduler.  Must hold only p->lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->noff, but that would
// break in the few places where a lock is held but
// there's no process.
void sched(void) {
  int intena;
  struct proc *p = myproc();

  if (!holding(&p->lock))
    panic("sched p->lock");
  if (mycpu()->noff != 1)
    panic("sched locks");
  if (p->state == RUNNING)
    panic("sched running");
  if (intr_get())
    panic("sched interruptible");

  intena = mycpu()->intena;
  swtch(&p->context, &mycpu()->context);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void yield(void) {
  struct proc *p = myproc();
  acquire(&p->lock);
  p->state = RUNNABLE;
  sched();
  release(&p->lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void forkret(void) {
  static int first = 1;

  // Still holding p->lock from scheduler.
  release(&myproc()->lock);

  if (first) {
    // File system initialization must be run in the context of a
    // regular process (e.g., because it calls sleep), and thus cannot
    // be run from main().
    fsinit(ROOTDEV);

    first = 0;
    // ensure other cores see first=0.
    __sync_synchronize();
  }

  usertrapret();
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void sleep(void *chan, struct spinlock *lk) {
  struct proc *p = myproc();

  // Must acquire p->lock in order to
  // change p->state and then call sched.
  // Once we hold p->lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup locks p->lock),
  // so it's okay to release lk.

  acquire(&p->lock); // DOC: sleeplock1
  release(lk);

  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  release(&p->lock);
  acquire(lk);
}

// Wake up all processes sleeping on chan.
// Must be called without any p->lock.
void wakeup(void *chan) {
  struct proc *p;

  for (p = proc; p < &proc[NPROC]; p++) {
    if (p != myproc()) {
      acquire(&p->lock);
      if (p->state == SLEEPING && p->chan == chan) {
        p->state = RUNNABLE;
      }
      release(&p->lock);
    }
  }
}

// Kill the process with the given pid.
// The victim won't exit until it tries to return
// to user space (see usertrap() in trap.c).
int kill(int pid) {
  struct proc *p;

  for (p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if (p->pid == pid) {
      p->killed = 1;
      if (p->state == SLEEPING) {
        // Wake process from sleep().
        p->state = RUNNABLE;
      }
      release(&p->lock);
      return 0;
    }
    release(&p->lock);
  }
  return -1;
}

void setkilled(struct proc *p) {
  acquire(&p->lock);
  p->killed = 1;
  release(&p->lock);
}

int killed(struct proc *p) {
  int k;

  acquire(&p->lock);
  k = p->killed;
  release(&p->lock);
  return k;
}

// Copy to either a user address, or kernel address,
// depending on usr_dst.
// Returns 0 on success, -1 on error.
int either_copyout(int user_dst, uint64 dst, void *src, uint64 len) {
  struct proc *p = myproc();
  if (user_dst) {
    return copyout(p->pagetable, dst, src, len);
  } else {
    memmove((char *)dst, src, len);
    return 0;
  }
}

// Copy from either a user address, or kernel address,
// depending on usr_src.
// Returns 0 on success, -1 on error.
int either_copyin(void *dst, int user_src, uint64 src, uint64 len) {
  struct proc *p = myproc();
  if (user_src) {
    return copyin(p->pagetable, dst, src, len);
  } else {
    memmove(dst, (char *)src, len);
    return 0;
  }
}

static void itoa(register int num, register char *const str,
                 register int const width) {
  int i = width - 1;

  // Fill with spaces initially
  for (int j = 0; j < width; j++) {
    str[j] = ' ';
  }

  // Handle zero case specifically
  if (num == 0) {
    str[i] = '0';
    return;
  }

  // Convert number to string from the end for non-zero numbers
  while (num > 0 && i >= 0) {
    str[i--] = '0' + (num % 10);
    num /= 10;
  }
}
static void putstat(register struct pstat const *const stat) {
  register int i = 0;

  printf("PID  | In Use | In Q | Waiting time | Running time | # Times "
         "Scheduled | Original Tickets | Current Tickets | q0  | q1\n");
  printf("-----|--------|------|--------------|--------------|-----------------"
         "--|------------------|-----------------|-----|-----\n");

  for (i = 0; i < NPROC; i += 1) {
    enum { BUFFER_SIZE = 150 };
    char line[BUFFER_SIZE] = {0};

    if (stat->pid[i] <= 0) {
      /* not an actual process; just skip it */
      continue;
    }

    memset(line, ' ', BUFFER_SIZE);
    line[BUFFER_SIZE - 1] = '\0';

    // Place each integer in the correct position
    itoa(stat->pid[i], line + 0, 3);
    itoa(stat->inuse[i], line + 8, 3);
    itoa(stat->inQ[i], line + 16, 3);
    itoa(stat->waiting_time[i], line + 29, 3);
    itoa(stat->running_time[i], line + 43, 3);
    itoa(stat->times_scheduled[i], line + 60, 3);
    itoa(stat->tickets_original[i], line + 80, 3);
    itoa(stat->tickets_current[i], line + 99, 3);
    itoa((int)stat->queue_ticks[i][0], line + 110, 3);
    itoa((int)stat->queue_ticks[i][1], line + 115, 3);

    // Print the formatted line
    printf("%s\n", line);
  }
}

void fill_pinfo(register struct pstat *const stat) {
  register struct proc *p = 0;
  register int i = 0;
  for (p = proc, i = 0; p < &proc[NPROC]; p += 1, i += 1) {
    acquire(&p->lock);
    if (p->state == UNUSED) {
      goto filldone;
    }
    stat->pid[i] = p->pid;

    if ((p->state == RUNNING) || (p->state == RUNNABLE)) {
      stat->inuse[i] = 1;
    } else {
      stat->inuse[i] = 0;
    }

    stat->inQ[i] = p->queue_index;

    acquire(&tickslock);
    stat->waiting_time[i] = ticks - p->last_sched_time;
    release(&tickslock);

    stat->times_scheduled[i] = p->times_scheduled;
    stat->tickets_original[i] = p->tickets_original;
    stat->tickets_current[i] = p->tickets_current;
    stat->queue_ticks[i][0] = p->queue_ticks[0];
    stat->queue_ticks[i][1] = p->queue_ticks[1];

  filldone:
    release(&p->lock);
  }
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void procdump(void) {
  // /* clang-format off */
  // static char const *states[] = {
  // [UNUSED]    =  "unused",
  // [USED]      =  "used",
  // [SLEEPING]  =  "sleep ",
  // [RUNNABLE]  =  "runble",
  // [RUNNING]   =  "run   ",
  // [ZOMBIE]    =  "zombie"
  // };
  // /* clang-format on */
  // struct proc *p;
  // char const *state;
  //
  // printf("\n");
  // for (p = proc; p < &proc[NPROC]; p++) {
  //   if (p->state == UNUSED)
  //     continue;
  //   if (p->state >= 0 && p->state < NELEM(states) && states[p->state])
  //     state = states[p->state];
  //   else
  //     state = "???";
  //   printf("%d %s %s", p->pid, state, p->name);
  //   printf("\n");
  // }

  auto struct pstat stat = {0};
  fill_pinfo(&stat);
  putstat(&stat);
}
