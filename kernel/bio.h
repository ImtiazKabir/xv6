#ifndef KERNEL_BIO_H_
#define KERNEL_BIO_H_

#include "common/types.h"

struct buf;

void binit(void);
struct buf *bread(uint dev, uint blockno);
void brelse(struct buf *b);
void bwrite(struct buf *b);
void bpin(struct buf *b);
void bunpin(struct buf *b);

#endif /* !KERNEL_BIO_H_ */
