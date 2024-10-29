#include "common/types.h"
#include "kernel/syscall.h"

static uint64 seed;

uint64 sys_seed(void) {
  seed = argraw(0);
  return 0;
}

void xorshift64(void) {
	uint64 x = seed;
	x ^= x << 13;
	x ^= x >> 7;
	x ^= x << 17;
	seed = x;
}

uint64 sys_rand(void) {
  xorshift64();
  return seed;
}

