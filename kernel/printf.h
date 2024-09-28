#ifndef KERNEL_PRINTF_H_
#define KERNEL_PRINTF_H_

int printf(char const *fmt, ...) __attribute__((format(printf, 1, 2)));
void panic(char const *s) __attribute__((noreturn));
void printfinit(void);

#endif /* !KERNEL_PRINTF_H_ */
