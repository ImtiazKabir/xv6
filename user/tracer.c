#include "ulib.h"
#include "usys.h"

#define STDERR 2

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

extern int main(int argc, char const **const argv) __attribute__((noreturn));

int main(register int const argc, register char const **const argv) {
  register int syscall_id = 0;

  if (argc < 3) {
    fprintf(STDERR, "usage: %s syscall_id command\n", argv[0u]);
    exit(EXIT_FAILURE);
  }

  syscall_id = atoi(argv[1]);
  if (syscall_id <= 0) {
    fprintf(STDERR, "Invalid syscall_id");
    exit(EXIT_FAILURE);
  }

  {
    register int const pid = fork();
    if (pid == 0) {
      (void)trace(syscall_id);
      (void)exec(argv[2u], &argv[2u]);
    } else {
      auto int wstatus = 0;

      (void)wait(&wstatus);

      if (wstatus != EXIT_SUCCESS) {
        printf("Trace failed");
        exit(wstatus);
      }
    }
  }

  exit(EXIT_SUCCESS);
}
