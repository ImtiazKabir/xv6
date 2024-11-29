#ifndef KERNEL_TVM_H_
#define KERNEL_TVM_H_

#include "kernel/proc.h"

int tvmcopy(pagetable_t, pagetable_t, uint64);
int tvmrcopy(pagetable_t, pagetable_t, uint64, uint64);
void tvmfree(pagetable_t pagetable, uint64 sz);
uint64 tvmdealloc(pagetable_t pagetable, uint64 oldsz, uint64 newsz);

int growthread(struct proc *, uint64, uint64);
void shrinkthread(struct proc *, uint64, uint64);

#endif /* !KERNEL_TVM_H_ */
