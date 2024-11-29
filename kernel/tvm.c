#include "tvm.h"
#include "common/riscv.h"
#include "kernel/printf.h"
#include "vm.h"

// shared memory for child thread
// returns 0 on success, -1 on failure.
// frees any allocated pages on failure.
int tvmcopy(pagetable_t old, pagetable_t new, uint64 sz) {
  /* Copied from uvmcopy(): */
  /* CHANGE: no new kalloc + memmove, mapping the direct physical address */

  pte_t *pte;
  uint64 pa, i;
  uint flags;

  for (i = 0; i < sz; i += PGSIZE) {
    if ((pte = walk(old, i, 0)) == 0)
      panic("thread_uvmcopy: pte should exist");
    if ((*pte & PTE_V) == 0)
      panic("thread_uvmcopy: page not present");
    pa = PTE2PA(*pte);
    flags = PTE_FLAGS(*pte);

    if (mappages(new, i, PGSIZE, (uint64)pa, flags) != 0) {
      goto err;
    }
  }
  return 0;

err:
  uvmunmap(new, 0, i / PGSIZE, 1);
  return -1;
}

// Free user memory pages,
// then free page-table pages.
void tvmfree(pagetable_t pagetable, uint64 sz) {
  /* Copied from uvmfree() */
  /* CHANGE: pass 0 to uvmunmap to stop it from freeing */
  if (sz > 0)
    uvmunmap(pagetable, 0, PGROUNDUP(sz) / PGSIZE, 0);
  freewalk(pagetable);
}
