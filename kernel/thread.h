#ifndef KERNEL_THREAD_H_
#define KERNEL_THREAD_H_

#include "common/types.h"
#include "proc.h"

uint64 sys_thread_create(void);
uint64 sys_thread_join(void);
uint64 sys_thread_exit(void);
void thread_freepagetable(pagetable_t pagetable, uint64 sz);

#endif /* KERNEL_THREAD_H_ */
