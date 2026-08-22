#include "common/pstat.h"

#include "printf.h"
#include "ulib.h"
#include "usys.h"

#define NPROC 64

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

extern int main(int argc, char const *const *argv) __attribute__((noreturn));

static void itoa(register int num, register char *const str,
                 register int const width) {
  int i = width - 1;

  // Fill with spaces initially
  for (int j = 0; j < width; j++) {
    str[j] = ' ';
  }

  // Handle zero case specifically
  if (num == 0) {
    str[i] = '0';
    return;
  }

  // Convert number to string from the end for non-zero numbers
  while (num > 0 && i >= 0) {
    str[i--] = '0' + (num % 10);
    num /= 10;
  }
}

static void putstat(register struct pstat const *const stat) {
  register int i = 0;

  printf("PID  | In Use | In Q | Waiting time | Running time | # Times "
         "Scheduled | Original Tickets | Current Tickets | q0  | q1\n");
  printf("-----|--------|------|--------------|--------------|-----------------"
         "--|------------------|-----------------|-----|-----\n");

  for (i = 0; i < NPROC; i += 1) {
    enum { BUFFER_SIZE = 150 };
    char line[BUFFER_SIZE] = {0};

    if (stat->pid[i] <= 0) {
      /* not an actual process; just skip it */
      continue;
    }

    memset(line, ' ', BUFFER_SIZE);
    line[BUFFER_SIZE - 1] = '\0';

    // Place each integer in the correct position
    itoa(stat->pid[i], line + 0, 3);
    itoa(stat->inuse[i], line + 8, 3);
    itoa(stat->inQ[i], line + 16, 3);
    itoa(stat->waiting_time[i], line + 29, 3);
    itoa(stat->running_time[i], line + 43, 3);
    itoa(stat->times_scheduled[i], line + 60, 3);
    itoa(stat->tickets_original[i], line + 80, 3);
    itoa(stat->tickets_current[i], line + 99, 3);
    itoa((int)stat->queue_ticks[i][0], line + 110, 3);
    itoa((int)stat->queue_ticks[i][1], line + 115, 3);

    // Print the formatted line
    printf("%s\n", line);
  }
}

int main(register int const argc, register char const *const *const argv) {
  auto struct pstat stat = {0};

  if (argc != 1) {
    fprintf(2, "Usage: %s\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  if (getpinfo(&stat) != 0) {
    fprintf(2, "Failed to retrieve stat informations\n");
    exit(EXIT_FAILURE);
  }

  getpinfo(&stat);
  putstat(&stat);

  exit(EXIT_SUCCESS);
}
