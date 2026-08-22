#include "common/types.h"
#include "kernel/syscall.h"

static uint64 seed = 2005041;

uint64 sys_seed(void) {
  seed = argraw(0);
  return 0;
}

void xorshift64(void) {
  register uint64 x = seed;
  x ^= x << 13;
  x ^= x >> 7;
  x ^= x << 17;
  seed = x;
}

uint64 sys_rand(void) {
  xorshift64();
  return seed;
}

uint choose(register uint64 const *const arr, register uint const len) {
  register uint64 accum = 0;
  register uint64 rv = 0;
  register uint i = 0;

  for (i = 0; i < len; i += 1) {
    accum += arr[i];
  }

  rv = sys_rand() % accum;

  accum = 0;
  for (i = 0; i < len; i += 1) {
    if ((rv >= accum) && (rv < accum + arr[i])) {
      return i;
    }
    accum += arr[i];
  }

  return (uint)-1;
}
