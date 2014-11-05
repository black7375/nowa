#ifndef JOINT_H
#define JOINT_H

#include "fibrile.h"
#include <stdint.h>

typedef struct _frame_t {
  void * top;
  void * btm;
  void * ptr;
} frame_t;

typedef struct _fibrile_joint_t {
  int lock;
  int count;
  frame_t * frame;
  struct _fibrile_joint_t * parent;
} joint_t;

typedef fibrile_data_t data_t;

extern joint_t _joint;

#endif /* end of include guard: JOINT_H */
