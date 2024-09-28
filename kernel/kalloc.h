#ifndef KERNEL_KALLOC_H_
#define KERNEL_KALLOC_H_

void *kalloc(void);
void kfree(void *pa);
void kinit(void);

#endif /* !KERNEL_KALLOC_H_ */
