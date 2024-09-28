#ifndef KERNEL_UART_H_
#define KERNEL_UART_H_

// uart.c
void uartinit(void);
void uartintr(void);
void uartputc(int c);
void uartputc_sync(int c);
int uartgetc(void);

#endif // !KERNEL_UART_H_
