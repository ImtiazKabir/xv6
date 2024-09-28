#ifndef KERNEL_RAMDISK_H_
#define KERNEL_RAMDISK_H_

struct buf;

// ramdisk.c
void ramdiskinit(void);
void ramdiskintr(void);
void ramdiskrw(struct buf *b);

#endif /* !KERNEL_RAMDISK_H_ */
