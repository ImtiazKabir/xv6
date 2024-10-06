#ifndef USER_PRINTF_H_
#define USER_PRINTF_H_

void fprintf(int fd, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));
void printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

#endif /* !USER_PRINTF_H_ */
