#ifndef CONDITION_H
#define CONDITION_H


#include "queue.h"

struct cv {
   queue_t wait_queue;
   int id;
   char* name;
};

void cv_create(struct cv* condition, char* name);
void cv_wait(struct cv* condition); 
void cv_signal(struct cv* condition);
void cv_release(struct cv* condition);
#endif