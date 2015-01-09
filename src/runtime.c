#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include "proc.h"
#include "safe.h"
#include "debug.h"
#include "param.h"

static pthread_t * _procs;
static void ** _stacks;
static int nprocs;

static void * __main(void * id)
{
  int tid = (int) (intptr_t) id;

  proc_start(tid, nprocs);
  return NULL;
}

int fibril_rt_nprocs(int n)
{
  return param_nprocs(n);
}

int fibril_rt_init(int n)
{
  param_init();

  nprocs = param_nprocs(n);
  DEBUG_DUMP(2, "fibril_rt_init:", (nprocs, "%d"));

  if (nprocs < 0) return -1;

  size_t stacksize = PARAM_STACK_SIZE;

  _procs = malloc(sizeof(pthread_t [nprocs]));
  _stacks = malloc(sizeof(void * [nprocs]));

  pthread_attr_t attrs[nprocs];
  int i;

  for (i = 1; i < nprocs; ++i) {
    SAFE_RZCALL(posix_memalign(&_stacks[i], PARAM_PAGE_SIZE, stacksize));
    pthread_attr_init(&attrs[i]);
    pthread_attr_setstack(&attrs[i], _stacks[i], stacksize);
    pthread_create(&_procs[i], &attrs[i], __main, (void *) (intptr_t) i);
    pthread_attr_destroy(&attrs[i]);
  }

  _procs[0] = pthread_self();
  SAFE_RZCALL(posix_memalign(&_stacks[0], PARAM_PAGE_SIZE, stacksize));

  register void * rsp asm ("r15");
  rsp = _stacks[0] + stacksize;

  __asm__ ( "xchg\t%0,%%rsp" : "+r" (rsp) );
  __main((void *) 0);
  __asm__ ( "xchg\t%0,%%rsp" : : "r" (rsp) );

  return 0;
}

int fibril_rt_exit()
{
  proc_stop();

  int i;

  for (i = 1; i < nprocs; ++i) {
    pthread_join(_procs[i], NULL);
    free(_stacks[i]);
  }

  free(_procs);
  free(_stacks);

  return 0;
}

