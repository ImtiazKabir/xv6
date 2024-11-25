#include "printf.h"
#include "ulib.h"
#include "usys.h"


void start(void *num) {
  printf("Id: %d\n", *(int *)num);
  exit(0);
}

int main(int argc, char *argv[]) {
  auto int id = 2005041;
  (void)argc, (void)argv;
  thread_create(start, &id, 0);
  exit(0);
}
