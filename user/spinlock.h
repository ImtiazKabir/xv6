#ifndef USER_SPINLOCK_H_
#define USER_SPINLOCK_H_

#include "common/types.h"

// Mutual exclusion lock.
struct thread_spinlock {
  volatile uint locked; // Is the lock held?
};

// spinlock.c
void thread_spin_init(struct thread_spinlock *lk);
void thread_spin_lock(struct thread_spinlock *lk);
void thread_spin_unlock(struct thread_spinlock *lk);

#endif /* !USER_SPINLOCK_H_ */
