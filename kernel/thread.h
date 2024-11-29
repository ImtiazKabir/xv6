#ifndef KERNEL_THREAD_H_
#define KERNEL_THREAD_H_

#include "common/types.h"
#include "proc.h"

int thread_create(uint64 start, uint64 arg, uint64 stack);
int thread_join(int thread_id);
void thread_exit(void);
void thread_freepagetable(pagetable_t pagetable, uint64 sz);

#endif /* !KERNEL_THREAD_H_ */
