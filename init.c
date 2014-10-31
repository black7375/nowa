#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sched.h>
#include <signal.h>
#include <unistd.h>

#include "tls.h"
#include "page.h"
#include "safe.h"
#include "util.h"
#include "debug.h"
#include "sched.h"
#include "stack.h"
#include "shmap.h"
#include "fibrili.h"

char __data_start, _end;
tls_t _tls;

int     _nprocs;
int  *  _pids;
void ** _stacks;

static int * _tls_files;

static void globe_init(int nprocs)
{
  shmap_init(nprocs);

  void * dat_start = PAGE_ALIGN_DOWN(&__data_start);
  void * dat_end   = PAGE_ALIGN_UP(&_end);

  DEBUG_PRINTI("data: %p ~ %p\n", dat_start, dat_end);
  SAFE_ASSERT(PAGE_ALIGNED(dat_start) && PAGE_ALIGNED(dat_end));

  void * tls_start = &_tls;
  void * tls_end   = tls_start + sizeof(_tls);

  DEBUG_PRINTI("tls: %p ~ %p\n", tls_start, tls_end);
  SAFE_ASSERT(PAGE_ALIGNED(tls_start) && PAGE_ALIGNED(tls_end));

  if (dat_start < tls_start) {
    shmap_copy(dat_start, tls_start - dat_start, "data");
  }

  if (tls_end < dat_end) {
    shmap_copy(tls_end, dat_end - tls_end, "data_2");
  }
}

static void tls_init(int id)
{
  char path[FILENAME_LIMIT];
  sprintf(path, "tls_%d", id);

  void * addr = &_tls;
  size_t size = sizeof(_tls);

  int file = shmap_copy(addr, size, path);
  _tls_files[id] = file;

  barrier(_nprocs);

  _deq.deqs = malloc(sizeof(tls_t *) * _nprocs);

  int i;
  for (i = 0; i < _nprocs; ++i) {
    if (i != id) {
      _deq.deqs[i] = shmap_mmap(NULL, size, _tls_files[i]);
    }
  }
}

static int child_main(void * id_)
{
  int id = (int) (size_t) id_;

  _tid = id;
  _pid = getpid();

  tls_init(id);
  stack_init_child(id);

  sched_work(id, _nprocs);
  return 0;
}

int fibril_init(int nprocs)
{
  _tid = 0;
  _pid = getpid();
  _nprocs = nprocs;
  _pids = malloc(sizeof(int) * nprocs);

  globe_init(nprocs);
  stack_init(nprocs);

  _tls_files = malloc(sizeof(int) * nprocs);
  _stacks = malloc(sizeof(void *) * nprocs);

  int i;
  for (i = 0; i < nprocs; ++i) {
    _stacks[i] = stack_new(NULL);
  }

  /** Create workers. */
  for (i = 1; i < nprocs; ++i) {
    SAFE_RETURN(_pids[i], clone(child_main, _stacks[i],
          CLONE_FS | CLONE_FILES | CLONE_IO | SIGCHLD,
          (void *) (size_t) i));
    DEBUG_PRINTI("clone: tid=%d pid=%d stack=%p\n", i, _pids[i], _stacks[i]);
  }

  tls_init(0);
  free(_tls_files);
  return 0;
}

