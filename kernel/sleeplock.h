#ifndef KERNEL_SLEEPLOCK_H_
#define KERNEL_SLEEPLOCK_H_
#include "common/types.h"
#include "spinlock.h"

// Long-term locks for processes
struct sleeplock {
  uint locked;        // Is the lock held?
  struct spinlock lk; // spinlock protecting this sleep lock

  // For debugging:
  char const *name; // Name of lock.
  int pid;          // Process holding lock
};

// sleeplock.c
void acquiresleep(struct sleeplock *lk);
void releasesleep(struct sleeplock *lk);
int holdingsleep(struct sleeplock *lk);
void initsleeplock(struct sleeplock *lk, char const *name);

#endif /* KERNEL_SLEEPLOCK_H_ */
