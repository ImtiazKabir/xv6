#ifndef COMMON_PSTAT_H_
#define COMMON_PSTAT_H_

#include "param.h"
#include "types.h"

struct pstat {
  int pid[NPROC];   // the process ID of each process
  int inuse[NPROC]; // whether this slot of the process table is being used (1
                    // or 0)
  int inQ[NPROC];   // which queue the process is currently in
  int waiting_time[NPROC]; // the time each process has spent waiting before
                           // being scheduled
  int running_time[NPROC]; // Number of times the process was scheduled before
                           // its time slice was used
  int times_scheduled[NPROC];  // the total number of times this process was
                               // scheduled
  int tickets_original[NPROC]; // the number of tickets each process originally
                               // had
  int tickets_current[NPROC];  // the number of tickets each process currently
                               // has
  uint queue_ticks[NPROC][2];  // the total number of ticks each process has
                               // spent in each queue
};

#endif /* !COMMON_PSTAT_H_ */
