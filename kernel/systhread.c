#include "common/types.h"

#include "syscall.h"
#include "thread.h"

uint64 sys_thread_create(void) {
  register uint64 const fcn = argraw(0);
  register uint64 const arg = argraw(1);
  register uint64 const stack = argraw(2);

  return (uint64)thread_create(fcn, arg, stack);
}

uint64 sys_thread_join(void) {
  int thread_id = (int)argraw(0);
  return (uint64)thread_join(thread_id);
}

uint64 sys_thread_exit(void) {
  thread_exit();
  return 0;
}
