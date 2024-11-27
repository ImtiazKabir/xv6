#ifndef USER_MUTEX_H_
#define USER_MUTEX_H_

#include "common/types.h"

// Mutual exclusion lock.
struct thread_mutex {
  volatile uint locked; // Is the lock held?
};

// spinlock.c
void thread_mutex_init(struct thread_mutex *lk);
void thread_mutex_lock(struct thread_mutex *lk);
void thread_mutex_unlock(struct thread_mutex *lk);

#endif /* !USER_MUTEX_H_ */

