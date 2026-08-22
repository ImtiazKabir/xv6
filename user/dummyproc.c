#include "printf.h"
#include "ulib.h"
#include "usys.h"

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#define CHILD_COUNT 4
#define CHILD_SLEEP_ITER_PERIOD 100000000
#define CHILD_SLEEP_DURATION 10

extern int main(int argc, char const *const *argv) __attribute__((noreturn));
static void cmain(register int const num_of_iterations)
    __attribute__((noreturn));

static void cmain(register int const num_of_iterations) {
  register int i = 0;
  for (i = 0; i < num_of_iterations; i += 1) {
    if (i % CHILD_SLEEP_ITER_PERIOD == 0) {
      sleep(CHILD_SLEEP_DURATION);
    }
  }
  exit(EXIT_SUCCESS);
}

int main(register int const argc, register char const *const *const argv) {
  register int num_of_tickets = 0;
  register int num_of_iterations = 0;
  register int i = 0;

  if (argc != 3) {
    fprintf(2, "usage: %s num_of_tickets num_of_iterations\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  num_of_tickets = atoi(argv[1]);
  num_of_iterations = atoi(argv[2]);

  printf("PARENT: Called with %d tickets %d iters\n", num_of_tickets, num_of_iterations);

  settickets(num_of_tickets);

  for (i = 0; i < CHILD_COUNT; i += 1) {
    int const pid = fork();
    if (pid == 0) {
      cmain(num_of_iterations);
    } else {
      printf("CHILD: process %d with %d tickets started for %d loops\n", pid, num_of_tickets, num_of_iterations);
    }
  }

  for (i = 0; i < num_of_iterations; i += 1) {
    /* do nothing */
  }

  /* Spec wise, I think dummyproc should wait for its children to complete */
  while (wait(0) != -1) {
  }

  printf("PARENT: exiting...\n");

  exit(EXIT_SUCCESS);
}
