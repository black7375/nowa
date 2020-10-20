#ifndef POOL_H
#define POOL_H



//#define POOL_WAIT_FREE
//#define POOL_LOCAL_POOLS



void pool_put(void * stack);
void * pool_take();

#endif /* end of include guard: POOL_H */
