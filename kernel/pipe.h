#ifndef KERNEL_PIPE_H_
#define KERNEL_PIPE_H_
#include "common/types.h"

struct file;
struct pipe;

int pipealloc(struct file **f0, struct file **f1);
void pipeclose(struct pipe *pi, int writable);
int piperead(struct pipe *pi, uint64 addr, int n);
int pipewrite(struct pipe *pi, uint64 addr, int n);

#endif /* !KERNEL_PIPE_H_ */
