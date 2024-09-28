#ifndef COMMON_PROCINFO_H_
#define COMMON_PROCINFO_H_

struct procInfo {
  int activeProcess; // # of processes in RUNNABLE and RUNNING state
  int totalProcess;  // # of total possible processes
  int memsize;       // in bytes; summation of all active process
  int totalMemSize;  // in bytes; all available physical Memory
};

#endif /* PROCINFO_H_ */
