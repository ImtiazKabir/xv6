#ifndef KERNEL_SWTCH_H_
#define KERNEL_SWTCH_H_

struct context;

// swtch.S
void swtch(struct context *old, struct context *new);

#endif // !KERNEL_SWTCH_H_
