#ifndef KERNEL_LOG_H_
#define KERNEL_LOG_H_

struct superblock;
struct buf;

void initlog(int dev, struct superblock *sb);
void log_write(struct buf *b);
void begin_op(void);
void end_op(void);

#endif /* KERNEL_LOG_H_ */
