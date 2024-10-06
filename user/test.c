#include "common/memlayout.h"

#include "printf.h"
#include "ulib.h"
#include "usys.h"

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  if (fork() == 0) {
    register char *const str = (char *)PSHARED;
    str[0] = 'h';
    str[1] = 'i';
    str[2] = '\0';
    exit(0);
  } else {
    register char const *const str = (char *)CSHARED;
    wait(0);
    printf("%s\n", str);
  }
  exit(0);
}
