#include "common/riscv.h"

#include "printf.h"
#include "ulib.h"
#include "usys.h"
#include "umalloc.h"
#include "spinlock.h"


extern int main(int argc, char const *const *argv) __attribute__((noreturn));

static int count = 2005041;
static struct thread_spinlock lk = {0};

static void start(register void *const arg) {
  (void)arg;
  thread_spin_lock(&lk);
  printf("%d\n", count);
  thread_spin_unlock(&lk);
  thread_exit();
}

int main(register int const argc, register char const *const argv[]) {
  auto int t1 = 0;
  register void *const stack1 = malloc(PGSIZE);

  auto int t2 = 0;
  register void *const stack2 = malloc(PGSIZE);

  (void)argc, (void)argv;

  thread_spin_init(&lk);

  t1 = thread_create(start, 0, stack1);
  t2 = thread_create(start, 0, stack2);

  (void)thread_join(t1);
  (void)thread_join(t2);

  free(stack1);
  free(stack2);

  exit(0);
}
