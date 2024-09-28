#ifndef KERNEL_STRING_H_
#define KERNEL_STRING_H_

#include "common/types.h"

// string.c
int memcmp(const void *v1, const void *v2, uint n);
void *memmove(void *dst, const void *src, uint n);
void *memset(void *dst, int c, uint n);
char *safestrcpy(char *dst, const char *src, int n);
int strlen(const char *s);
int strncmp(const char *p, const char *q, uint n);
char *strncpy(char *s, const char *t, int n);

#endif // !KERNEL_STRING_H_
