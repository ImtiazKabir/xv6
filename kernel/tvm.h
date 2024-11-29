#ifndef KERNEL_TVM_H_
#define KERNEL_TVM_H_

#include "kernel/proc.h"

int tvmcopy(pagetable_t, pagetable_t, uint64);
void tvmfree(pagetable_t pagetable, uint64 sz);

#endif /* !KERNEL_TVM_H_ */
