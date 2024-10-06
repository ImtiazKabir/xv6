#ifndef USER_ULIB_H_
#define USER_ULIB_H_

#include "common/stat.h"
#include "common/types.h"

uint64 syscall(int n, uint64, uint64, uint64, uint64, uint64);

int stat(const char *n, struct stat *st);

char *strcpy(char *dst, const char *src);
char *strchr(const char *str, char c);
int strcmp(const char *p, const char *q);
uint strlen(const char *str);

void *memmove(void *vdst, const void *vsrc, int n);
void *memset(void *dst, int c, uint n);
int memcmp(const void *s1, const void *s2, uint n);
void *memcpy(void *dst, const void *src, uint n);

char *gets(char *buf, int max);
int atoi(const char *numstr);

#endif /* !USER_ULIB_H_ */
