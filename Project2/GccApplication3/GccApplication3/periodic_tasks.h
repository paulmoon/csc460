#ifndef __PERIODIC_TASKS_H__
#define __PERIODIC_TASKS_H__

#include "kernel.h"

#ifdef __cplusplus
extern "C" {
#endif

void periodic_init();
void periodic_update_ticker(task_descriptor_t* c);
void periodic_push(queue_t* q,periodic_block_t* b);
periodic_block_t* periodic_pop(queue_t* q);

#ifdef __cplusplus
}
#endif

#endif
