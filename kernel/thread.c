#include "kernel/proc.h"
#include "kernel/printf.h"
#include "syscall.h"

#define FAKE_RETURN_ADDRESS 0xffffffff

uint64 sys_thread_create(void) {
  struct trapframe *trapframe = myproc()->trapframe;
  argaddr(0, &(trapframe->epc));
  argaddr(1, &(trapframe->a0));
  trapframe->ra = FAKE_RETURN_ADDRESS;
  return trapframe->a0;
}

uint64 sys_thread_join(void) {
  return 0;
}

uint64 sys_thread_exit(void) {
  return 0;
}

