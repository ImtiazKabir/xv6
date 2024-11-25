#include "common/riscv.h"

#include "printf.h"
#include "ulib.h"
#include "usys.h"
#include "umalloc.h"


void start(void *num) {
  printf("Arg: %d\n", *(int *)num);
  sleep(100);
  thread_exit();
}

int main(int argc, char *argv[]) {
  auto int num = 2005041;
  register int tid = 0;

  void *stack = malloc(PGSIZE);
  (void)argc, (void)argv;
  tid = thread_create(start, &num, stack);
  thread_join(tid);

  exit(0);
}
