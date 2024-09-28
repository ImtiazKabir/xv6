#ifndef KERNEL_TRAP_H_
#define KERNEL_TRAP_H_

#include "common/types.h"
#include "kernel/spinlock.h"

struct spinlock;

// trap.c
extern uint ticks;
extern struct spinlock tickslock;

void trapinit(void);
void trapinithart(void);
void usertrapret(void);

#endif // !KERNEL_TRAP_H_
