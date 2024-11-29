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
      panic("tvmcopy: pte should exist");
    if ((*pte & PTE_V) == 0)
      panic("tvmcopy: page not present");
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

// shared memory for child thread
// returns 0 on success, -1 on failure.
// frees any allocated pages on failure.
int tvmrcopy(pagetable_t old, pagetable_t new, uint64 oldsz, uint64 newsz) {
  /* Copied from tvmcopy(): */
  /* CHANGE: loop from oldsz to newsz */

  pte_t *pte;
  uint64 pa, i;
  uint flags;

  for (i = oldsz; i < newsz; i += PGSIZE) {
    if ((pte = walk(old, i, 0)) == 0)
      panic("tvmrcopy: pte should exist");
    if ((*pte & PTE_V) == 0)
      panic("tvmrcopy: page not present");
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

// Deallocate user pages to bring the process size from oldsz to
// newsz.  oldsz and newsz need not be page-aligned, nor does newsz
// need to be less than oldsz.  oldsz can be larger than the actual
// process size.  Returns the new process size.
uint64 tvmdealloc(pagetable_t pagetable, uint64 oldsz, uint64 newsz) {
  if (newsz >= oldsz)
    return oldsz;

  if (PGROUNDUP(newsz) < PGROUNDUP(oldsz)) {
    int npages = (PGROUNDUP(oldsz) - PGROUNDUP(newsz)) / PGSIZE;
    uvmunmap(pagetable, PGROUNDUP(newsz), npages, 0);
  }

  return newsz;
}

int growthread(struct proc *parent, uint64 oldsz, uint64 newsz) {
  struct proc *p = 0;
  for (p = proc; p < &proc[NPROC]; p++) {
    if (p == parent) {
      continue;
    }
    acquire(&p->lock);
    if (p->memlock == parent->memlock) {
      if (tvmrcopy(parent->pagetable, p->pagetable, oldsz, newsz) < 0) {
        release(&p->lock);
        return -1;
      }
      p->sz = parent->sz;
    }
    release(&p->lock);
  }
  return 0;
}

void shrinkthread(struct proc *parent, uint64 oldsz, uint64 newsz) {
  struct proc *p;
  for (p = proc; p < &proc[NPROC]; p++) {
    if (p == parent) {
      continue;
    }
    acquire(&p->lock);
    if (p->memlock == parent->memlock) {
      p->sz = tvmdealloc(p->pagetable, oldsz, newsz);
    }
    release(&p->lock);
  }
}
