#ifndef KERNEL_SPINLOCK_H_
#define KERNEL_SPINLOCK_H_
#include "common/types.h"

// Mutual exclusion lock.
struct spinlock {
  volatile uint locked; // Is the lock held?

  // For debugging:
  char const *name; // Name of lock.
  struct cpu *cpu;  // The cpu holding the lock.
};

// spinlock.c
void acquire(struct spinlock *lk);
int holding(struct spinlock *lk);
void initlock(struct spinlock *lk, char const *name);
void release(struct spinlock *lk);
void push_off(void);
void pop_off(void);

#endif /* !KERNEL_SPINLOCK_H_ */
