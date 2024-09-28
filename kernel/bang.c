#include "common/types.h"
#include "common/util.h"
#include "proc.h"
#include "string.h"
#include "syscall.h"
#include "vm.h"

uint64 sys_getlast(void) {
  auto uint64 dst = 0;
  register struct proc *const p = myproc();
  auto int i = 0u;
  auto char const *last_command = 0;

  argaddr(0, &dst);
  argint(1, (int *)&i);

  i = (p->command_index - i) % NELEM(p->command_history);
  last_command = p->command_history[i];

  (void)copyout(p->pagetable, dst, (void *)last_command,
                strlen((char *)last_command) + 1u);
  return 0u;
}

uint64 sys_setlast(void) {
  register struct proc *const p = myproc();
  enum { COMMAND_LEN = sizeof(p->command_history[0]) };
  auto char last_command[COMMAND_LEN] = {0};

  (void)argstr(0, (char *)last_command, sizeof(last_command));

  (void)safestrcpy((char *)p->command_history[p->command_index], last_command,
                   COMMAND_LEN);
  p->command_index = (p->command_index + 1u) % NELEM(p->command_history);

  return 0u;
}
