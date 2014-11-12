#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/syscall.h>

#include "page.h"
#include "safe.h"
#include "util.h"
#include "tlmap.h"
#include "config.h"
#include "fibrili.h"

typedef struct _shmap_t {
  struct _shmap_t * next;
  void * addr;
  size_t size;
  off_t  off;
  union {
    int   sh;
    int * tl;
  } file;
  int    otid;
  int    lock;
} shmap_t __attribute ((aligned (sizeof(void *))));

char __data_start, _end;
char _fibril_shm_start, _fibril_shm_end;

static struct sigaction _default_sa;

static shmap_t * find(shmap_t * head, void * addr)
{
  static shmap_t dummy = { 0 };

  shmap_t * map = head ? head : &dummy;
  shmap_t * next;

  while ((next = map->next) && next->addr <= addr) {
    map = next;
  }

  DEBUG_ASSERT(map != NULL);
  return map;
}

void shmap_create(void * addr, size_t size, int file, off_t off)
{
  static shmap_t maps[N_SHMAP_ENTRIES];
  static size_t next = 0;

  shmap_t * prev = NULL;
  shmap_t * curr;

  /** Find the mapping before the current one. */
  do {
    prev = find(prev, addr);
    lock(&prev->lock);

    if (prev->next && prev->next->addr <= addr) {
      unlock(&prev->lock);
    } else break;
  } while (1);

  if (prev->addr == addr && prev->size == size) {
    /** Replace the previous map. */
    if (prev->otid == TID) {
      prev->file.sh = file;
    }

    /** Fill thread local map. */
    else if (prev->otid == -1) {
      prev->file.tl[TID] = file;
    }

    /** Make a thread local mapping. */
    else {
      int * files = malloc(sizeof(int [_nprocs]));
      files[prev->otid] = prev->file.sh;
      files[TID] = file;

      prev->file.tl = files;
      prev->otid = -1;
    }

    prev->off = off;
  } else {
    curr = &maps[fadd(&next, 1)];

    if (prev->addr == addr && size < prev->size) {
      /** Replace the first part of the previous map. */
      curr->file.sh = prev->file.sh;
      curr->addr = addr + size;
      curr->size = prev->size - size;

      prev->file.sh = file;
      prev->addr = addr;
      prev->size = size;
    } else {
      curr->file.sh = file;
      curr->addr = addr;
      curr->size = size;

      /** Replace the last part of the previous map. */
      if (addr < prev->addr + prev->size) {
        prev->size = addr - prev->addr;
      }
    }

    curr->off = off;
    curr->next = prev->next;
    prev->next = curr;
  }

  unlock(&prev->lock);
}

static void * shmap(void * addr, size_t size, int file, off_t off)
{
  static int prot = PROT_READ | PROT_WRITE;
  int flag = MAP_SHARED;

  if (NULL != addr) {
    flag |= MAP_FIXED;
  }

  addr = mmap(addr, size, prot, flag, file, off);

  DEBUG_DUMP(3, "shmap:", (file, "%d"), (addr, "%p"),
      (size, "%ld"), (off, "%ld"));
  return addr;
}

static void segfault_sa(int signum, siginfo_t * info, void * ctxt)
{
  DEBUG_ASSERT(signum == SIGSEGV);

  void * addr = info->si_addr;
  shmap_t * map = find(NULL, addr);

  DEBUG_DUMP(1, "segfault_sa:", (addr, "%p"), (map->addr, "%p"),
      (map->size, "%ld"), (map->file.sh, "%d"));

  if (map->addr <= addr && map->addr + map->size > addr) {
    shmap(map->addr, map->size, map->file.sh, map->off);
  } else {
    DEBUG_BREAK(1);
    sigaction(SIGSEGV, &_default_sa, NULL);
    raise(signum);
  }
}

int shmap_open(size_t size, const char * name)
{
  static size_t suffix = 0;
  static char prefix[] = "fibril";

  char path[FILENAME_LIMIT];

  if (name) {
    sprintf(path, "/%s.%d.%s" , prefix, PID, name);
  } else {
    sprintf(path, "/%s.%d.%ld", prefix, PID, fadd(&suffix, 1));
  }

  int file;
  SAFE_NNCALL(file = shm_open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR));
  SAFE_NNCALL(ftruncate(file, size));
  SAFE_NNCALL(shm_unlink(path));

  DEBUG_DUMP(3, "shmap_open:", (path, "%s"), (file, "%d"), (size, "%ld"));
  return file;
}

void * shmap_mmap(void * addr, size_t size, int file)
{
  addr = shmap(addr, size, file, 0);
  shmap_create(addr, size, file, 0);

  return addr;
}

int shmap_copy(void * addr, size_t size, const char * name)
{
  /** Map the shm to a temporary address to copy the content of the pages. */
  int    shm = shmap_open(size, name);
  void * buf = shmap(NULL, size, shm, 0);

  memcpy(buf, addr, size);
  SAFE_NNCALL(munmap(buf, size));
  shmap_mmap(addr, size, shm);

  return shm;
}

void shmap_init(int nprocs)
{
  void * addr = PAGE_ALIGN_DOWN(&__data_start);
  size_t size = PAGE_ALIGN_UP(&_end) - addr;

  DEBUG_DUMP(2, "shmap_init (app):", (addr, "%p"), (size, "%ld"));
  DEBUG_ASSERT(PAGE_ALIGNED(addr) && PAGE_DIVISIBLE(size));

  shmap_copy(addr, size, "data");

  addr = &_fibril_shm_start;
  size = (void *) &_fibril_shm_end - addr;

  DEBUG_DUMP(2, "shmap_init (lib):", (addr, "%p"), (size, "%ld"));
  DEBUG_ASSERT(PAGE_ALIGNED(addr) && PAGE_DIVISIBLE(size) && size > 0);

  shmap_copy(addr, size, "shared");

  /** Setup SIGSEGV signal handler. */
  struct sigaction sa;
  sa.sa_sigaction = segfault_sa;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO;

  sigaction(SIGSEGV, &sa, &_default_sa);
}


