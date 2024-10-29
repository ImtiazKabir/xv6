#ifndef USER_USYS_H_
#define USER_USYS_H_

#include "common/types.h"

struct procInfo;
struct stat;

// system calls
int fork(void);
int exit(int status) __attribute__((noreturn));
int wait(int *wstatus);
int pipe(int *pipefd);
int write(int fd, const void *buf, int count);
int read(int fd, void *buf, int count);
int close(int fd);
int kill(int pid);
int exec(const char *path, char const **argv);
int open(const char *path, int flags);
int mknod(const char *path, short mode, short dev);
int unlink(const char *path);
int fstat(int fd, struct stat *statbuf);
int link(const char *old, const char *new);
int mkdir(const char *path);
int chdir(const char *path);
int dup(int oldfd);
int getpid(void);
char *sbrk(int sz);
int sleep(int tck);
int uptime(void);
int trace(int sysid);
int info(struct procInfo *in);
int getlast(char *dest, int index);
int setlast(char const *cmd);
void seed(uint64 seed);
uint64 rand(void);

#endif /* !USER_USYS_H_ */
