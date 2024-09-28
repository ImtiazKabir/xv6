#ifndef KERNEL_VIRTIO_DISK_H_
#define KERNEL_VIRTIO_DISK_H_

struct buf;

// virtio_disk.c
void virtio_disk_init(void);
void virtio_disk_rw(struct buf *b, int write);
void virtio_disk_intr(void);

#endif // !KERNEL_VIRTIO_DISK_H_
