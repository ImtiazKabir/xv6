#ifndef COMMON_PARAM_H_
#define COMMON_PARAM_H_

#define NPROC 64                  // maximum number of processes
#define NCPU 8                    // maximum number of CPUs
#define NOFILE 16                 // open files per process
#define NFILE 100                 // open files per system
#define NINODE 50                 // maximum number of active i-nodes
#define NDEV 10                   // maximum major device number
#define ROOTDEV 1                 // device number of file system root disk
#define MAXARG 32                 // max exec arguments
#define MAXOPBLOCKS 10            // max # of blocks any FS op writes
#define LOGSIZE (MAXOPBLOCKS * 3) // max data blocks in on-disk log
#define NBUF (MAXOPBLOCKS * 3)    // size of disk block cache
#define FSSIZE 2000               // size of file system in blocks
#define MAXPATH 128               // maximum file path name

#define LOTTERY_QUEUE 0     // lottery queue index
#define ROUND_ROBIN_QUEUE 1 // round robin queue index
#define DEFAULT_TICKETS 10  // default original lottery tickets of proc
#define TIME_LIMIT_0 2      // lottery queue time
#define TIME_LIMIT_1 4      // round robin queue time
#define WAIT_THRESH 6       // after this many ticks, p gets promoted

#define DEFAULT_QUEUE LOTTERY_QUEUE

#endif /* !COMMON_PARAM_H_ */

