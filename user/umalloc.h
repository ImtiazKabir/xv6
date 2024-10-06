#ifndef USER_UMALLOC_H_
#define USER_UMALLOC_H_

#include "common/types.h"

void *malloc(uint nbytes);
void free(void *ap);

#endif /* !USER_UMALLOC_H_ */
