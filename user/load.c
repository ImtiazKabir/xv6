#include "common/memlayout.h"
#include "common/procinfo.h"

#include "printf.h"
#include "ulib.h"
#include "umalloc.h"
#include "usys.h"

#define STDOUT 1
#define STDERR 2

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

extern int main(int argc, char const *const *argv) __attribute__((noreturn));

static void init(int lockoffset) {
  register int *const lk = (int *)(((char *)CSHARED) + lockoffset);
  *lk = 0;
}

static void pacq(int lockoffset) {
  register int *const lk = (int *)(((char *)PSHARED) + lockoffset);
  while (__sync_lock_test_and_set(lk, 1) != 0) {
  }
  __sync_synchronize();
}

static void cacq(int lockoffset) {
  register int *const lk = (int *)(((char *)CSHARED) + lockoffset);
  while (__sync_lock_test_and_set(lk, 1) != 0) {
  }
  __sync_synchronize();
}

static void prel(int lockoffset) {
  register int *const lk = (int *)(((char *)PSHARED) + lockoffset);
  __sync_synchronize();
  __sync_lock_release(lk);
}

static void crel(int lockoffset) {
  register int *const lk = (int *)(((char *)CSHARED) + lockoffset);
  __sync_synchronize();
  __sync_lock_release(lk);
}

static struct procInfo get_current_system_information(void) {
  auto struct procInfo proc_info = {0};
  (void)info(&proc_info);
  return proc_info;
}

static void printMB(register int const bytes, register int const n) {
  register int const unit_mb = 1 << 20u;
  register int const mb = bytes / unit_mb;
  register int remainder = bytes % unit_mb;
  register int i = 0;

  printf("%d.", mb);

  for (i = 0; i < n; i++) {
    remainder = (remainder * 10);
    int digit = remainder / unit_mb;
    printf("%d", digit);
    remainder %= (1024 * 1024);
  }
}

static void print_proc_info(register struct procInfo const proc_info) {
  printf("Current system information:\n");
  printf("Processes: %d/%d\n", proc_info.activeProcess, proc_info.totalProcess);
  printf("RAM: ");
  printMB(proc_info.memsize, 3);
  printf("/");
  printMB(proc_info.totalMemSize, 1);
  printf(" (in MB)\n");
}

static void handleChild(register uint const allocation_amount)
    __attribute__((noreturn));

static void handleChild(register uint const allocation_amount) {
  register void *const mem = malloc(allocation_amount);

  pacq(0);
  {
    printf("Child is created\n");
    printf("Child allocated %u bytes\n", allocation_amount);
    printf("Child is going to sleep\n");
  }
  prel(0);

  sleep(50);
  free(mem);
  exit(EXIT_SUCCESS);
}

static void load(register int const child_count,
                 register uint const allocation_amount) {
  register int i = 0;

  init(0);

  for (i = 0; i < child_count; i += 1) {
    if (fork() == 0) {
      handleChild(allocation_amount);
    }
  }
  cacq(0);
  { printf("Parent is going to sleep\n"); }
  crel(0);

  sleep(10);

  cacq(0);
  {
    printf("Parent is waking up\n");
    print_proc_info(get_current_system_information());
  }
  crel(0);

  while (wait(0) == 0) {
  }
}

int main(register int const argc, register char const *const *const argv) {
  if (argc != 3) {
    fprintf(STDERR, "usage: %s child_count allocation_amount\n", argv[0u]);
    exit(EXIT_FAILURE);
  }

  {
    register int child_count = 0u;
    register int allocation_amount = 0u;
    child_count = atoi(argv[1u]);
    if (child_count <= 0) {
      fprintf(STDERR, "Invalid child_count\n");
      exit(EXIT_FAILURE);
    }

    allocation_amount = atoi(argv[2u]);
    if (allocation_amount <= 0) {
      fprintf(STDERR, "Invalid allocation amount\n");
      exit(EXIT_FAILURE);
    }

    load(child_count, (uint)allocation_amount);
  }

  exit(EXIT_SUCCESS);
}
