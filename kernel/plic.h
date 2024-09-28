#ifndef KERNEL_PLIC_H_
#define KERNEL_PLIC_H_

// plic.c
void plicinit(void);
void plicinithart(void);
int plic_claim(void);
void plic_complete(int irq);

#endif // !KERNEL_PLIC_H_
