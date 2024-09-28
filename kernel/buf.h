#ifndef KERNEL_BUF_H_
#define KERNEL_BUF_H_
#include "common/fs.h"
#include "sleeplock.h"
#include "common/types.h"

struct buf {
  int valid; // has data been read from disk?
  int disk;  // does disk "own" buf?
  uint dev;
  uint blockno;
  struct sleeplock lock;
  uint refcnt;
  struct buf *prev; // LRU cache list
  struct buf *next;
  uchar data[BSIZE];
};
#endif /* !KERNEL_BUF_H_ */
