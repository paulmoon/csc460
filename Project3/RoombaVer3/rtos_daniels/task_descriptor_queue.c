/**
 * \file	Task_Descriptor_Queue.c
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

#include "kernel.h"
#include "Task_Descriptor_Queue.h"

/** Argument for adding sleeping tasks.  Available from kernel.h */
extern volatile unsigned int kernal_request_sleep_ticks;

/**
 * @brief Add a task the end of the queue
 *
 * @param queue_ptr the queue to insert in
 * @param task_to_add the task descriptor to add
 */
void enqueue(volatile queue_t* queue_ptr, task_descriptor_t * task_to_add) {

	task_to_add->next = NULL;

	if (queue_ptr->head == NULL) {
		/* empty queue */
		queue_ptr->head = task_to_add;
		queue_ptr->tail = task_to_add;
	} else {
		/* put task at the back of the queue */
		queue_ptr->tail->next = task_to_add;
		queue_ptr->tail = task_to_add;
	}
}

/**
 * @brief Add a task the front of the queue
 *
 * @param queue_ptr the queue to insert in
 * @param task_to_add the task descriptor to add
 */
void enqueue_front(volatile queue_t* queue_ptr, task_descriptor_t* task_to_add) {

	task_to_add->next = NULL;
	if (queue_ptr->head == NULL) {
		/* empty queue */
		queue_ptr->head = task_to_add;
		queue_ptr->tail = task_to_add;
	} else {
		//TEST_BLIP(BLACK);
		/* put task at the front of the queue */
		task_to_add->next=queue_ptr->head;
		queue_ptr->head = task_to_add;
	}
}

// We will likely have to change this function!
/**
 * @brief Add a task to the sleep queue
 *
 * @param queue_ptr the queue to insert in
 * @param task_to_add the task descriptor to add
 */
void sleep_enqueue(volatile queue_t* queue_ptr, task_descriptor_t* task_to_add) {

	task_descriptor_t* next = queue_ptr->head;
	task_descriptor_t* prev = NULL;
	unsigned int tick_count = 0;
	if (queue_ptr->head == NULL) {
		queue_ptr->head = task_to_add;
		task_to_add->sleep_next = NULL;
		task_to_add->diff_ticks_remaining = kernal_request_sleep_ticks;
		return;
	}

	while (next != NULL) {
		if ((tick_count + next->diff_ticks_remaining)
				>= kernal_request_sleep_ticks) {
			task_to_add->sleep_next = next;
			task_to_add->diff_ticks_remaining = kernal_request_sleep_ticks
					- tick_count;
			next->diff_ticks_remaining -= task_to_add->diff_ticks_remaining;
			if (prev == NULL) {
				queue_ptr->head = task_to_add;
			} else {
				prev->sleep_next = task_to_add;
			}
			break;
		}
		tick_count += next->diff_ticks_remaining;
		prev = next;
		next = next->sleep_next;
	}
	if (next == NULL) {
		prev->sleep_next = task_to_add;
		task_to_add->sleep_next = NULL;
		queue_ptr->tail = task_to_add;
		task_to_add->diff_ticks_remaining = kernal_request_sleep_ticks
				- tick_count;
	}
}

/**
 * @brief Removes from the sleep queue when time is up
 * @param queue_ptr pointer to the sleep queue
 *
 */
task_descriptor_t* sleep_dequeue(volatile queue_t* queue_ptr) {

	task_descriptor_t* task_ptr = queue_ptr->head;

	if (queue_ptr->head != NULL) {
		queue_ptr->head = queue_ptr->head->sleep_next;
		task_ptr->sleep_next = NULL;
	}

	return task_ptr;
}

/**
 * @brief Pops head of queue and returns it.
 *
 * @param queue_ptr the queue to pop
 * @return the popped task descriptor
 */
task_descriptor_t* dequeue(volatile queue_t* queue_ptr) {

	task_descriptor_t* task_ptr = queue_ptr->head;

	if (queue_ptr->head != NULL) {
		queue_ptr->head = queue_ptr->head->next;
		task_ptr->next = NULL;
	}

	return task_ptr;
}

void remove_from_sleep_queue(volatile queue_t* queue_ptr, task_descriptor_t* task_to_remove){
	
	if(queue_ptr->head == NULL) return;
	
	task_descriptor_t* td = queue_ptr->head;
	
	if(td == task_to_remove){
		queue_ptr->head = td->sleep_next;
		
		if(queue_ptr->head == NULL){
			queue_ptr->tail = NULL;
		} else {
			// Need to update the following tasks sleep value
			queue_ptr->head->diff_ticks_remaining += task_to_remove->diff_ticks_remaining;
		}
		return;
	}
		
	while(td != NULL){
		
		if(td->sleep_next == task_to_remove){
			td->sleep_next = td->sleep_next->sleep_next;
			if(td->sleep_next == NULL){				
				queue_ptr->tail = td;
			} else {
				// Need to update the following tasks sleep value
				td->sleep_next->diff_ticks_remaining += task_to_remove->diff_ticks_remaining;
			}
			break;
		}
		td = td->sleep_next;
	}
}