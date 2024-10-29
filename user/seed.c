#include "common/types.h"
#include "usys.h"
#include "printf.h"
#include "ulib.h"

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

extern int main(int argc, char const *const *argv) __attribute__((noreturn));

int main(register int const argc, register char const *const *const argv) {
  if (argc != 2) {
    fprintf(2, "Usage: %s seed\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  seed((uint64)atoi(argv[1]));

  exit(EXIT_SUCCESS);
}

