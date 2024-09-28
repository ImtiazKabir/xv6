#ifndef KERNEL_FS_H_
#define KERNEL_FS_H_
#include "common/fs.h"
#include "common/stat.h"

struct inode;

void fsinit(int dev);
int dirlink(struct inode *dp, char *name, uint inum);
struct inode *dirlookup(struct inode *dp, char *name, uint *poff);
struct inode *ialloc(uint dev, short type);
struct inode *idup(struct inode *ip);
void iinit(void);
void ilock(struct inode *ip);
void iput(struct inode *ip);
void iunlock(struct inode *ip);
void iunlockput(struct inode *ip);
void iupdate(struct inode *ip);
int namecmp(const char *s, const char *t);
struct inode *namei(char const *path);
struct inode *nameiparent(char *path, char *name);
int readi(struct inode *ip, int user_dst, uint64 dst, uint off, uint n);
void stati(struct inode *ip, struct stat *stat);
int writei(struct inode *ip, int, uint64 user_src, uint src, uint n);
void itrunc(struct inode *ip);

#endif // !KERNEL_FS_H_
