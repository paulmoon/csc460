/**
 * \file	Task_Descriptor_Queue.h
 * \brief	Provides all queue functionality for task descriptors.
 *			
 *			Some queue functionality originally written by Scott Craig, Justin Tanner 
 *			and Matt Campbell. Their contributions will be cited in the function brief 
 *			where appropriate.
 *
 *	@date 07/13
 *
 *	@author	Gordon Meyer
 *	@author Daniel McIlvaney
 *	@author Cale McNulty
 */ 

#ifndef TASK_DESCRIPTOR_QUEUE_H_
#define TASK_DESCRIPTOR_QUEUE_H_

/**
 * @brief Contains pointers to head and tail of a linked list.
 */
typedef struct {
	/** The first item in the queue. NULL if the queue is empty. */
	task_descriptor_t* head;
	/** The last item in the queue. Undefined if the queue is empty. */
	task_descriptor_t* tail;
} queue_t;

/** Functions */
void enqueue(volatile queue_t* queue_ptr, task_descriptor_t* task_to_add);
void enqueue_front(volatile queue_t* queue_ptr, task_descriptor_t* task_to_add);
task_descriptor_t* dequeue(volatile queue_t* queue_ptr);
void sleep_enqueue(volatile queue_t*, task_descriptor_t*);
task_descriptor_t* sleep_dequeue(volatile queue_t*);
void remove_from_sleep_queue(volatile queue_t* queue_ptr, task_descriptor_t* task_to_remove);

// list of queues
/** Queue of unused tasks */
volatile queue_t dead_pool_queue;

/** Queue of ready round robin tasks */
volatile queue_t rr_queue;

/** Queue of ready system tasks */
volatile queue_t system_queue;

/** Queue of sleeping periodic tasks.  
In the future a 'Task_Sleep' function will be implemented */
volatile queue_t sleep_queue;

#endif /* TASK_DESCRIPTOR_QUEUE_H_ */