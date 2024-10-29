#include "usys.h"
#include "printf.h"

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

extern int main(int argc, char const *const *argv) __attribute__((noreturn));

int main(register int const argc, register char const *const *const argv) {
  if (argc != 1) {
    fprintf(2, "Usage: %s\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  printf("%lu\n", rand());

  exit(EXIT_SUCCESS);
}

