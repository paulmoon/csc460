/**
* \file   kernel.c
*
* \brief Operating systems kernel functionality and data structures.
*
*		Some of this code was originally written by Scott Craig, Justin Tanner
*		and Matt Campbell. Their contributions will be cited in the function brief
*		where appropriate.
*
*	@date 07/13
*
*	@author	Gordon Meyer
*	@author Daniel McIlvaney
*	@author Cale McNulty
*/

#include "kernel.h"
#include "task_descriptor_queue.h"
#include "error_code.h"

#if defined MEGA_2560	// 2560
	#pragma message("\tMEGA 2560	 Using 17 bit addresses")
	#define STACK_CONTEXT_SIZE	(32 + 1 + 1 + 3 + 3)
	#define TIMER_COMPARE_VEC	TIMER5_COMPA_vect
	
#elif defined XPLAINED		// XPlained
	#pragma message("\tXPlained		Using 16 bit addresses")
	#define STACK_CONTEXT_SIZE	(32 + 1 + 2 + 2)
	#define TIMER_COMPARE_VEC	TIMER2_COMPA_vect
	
#endif

/* File Scope Prototypes ***************************************************/
static void kernel_dispatch(void);
static void kernel_handle_request(void);
static void exit_kernel(void) __attribute((noinline, naked));
static void kernel_update_ticker(void);
static void kernel_terminate_task(void);
static uint8_t has_execution_time_elapsed(task_descriptor_t *task);

/* events */
static void kernel_event_wait(void);
static void kernel_event_clear(void);
static void kernel_event_signal(void);

/* Members *****************************************************************/

/* Initialization of variables accessible externally through kernel.h **/
task_descriptor_t* cur_task = NULL;
task_descriptor_t* idle_task  = &task_desc[MAXPROCESS];
volatile unsigned int cur_ticks = 0;
uint8_t volatile error_msg = ERR_RUN_1_USER_CALLED_OS_ABORT;
task_descriptor_t* current_periodic_task = NULL;
volatile uint16_t events_state = 0;

/* Queues from task_descriptor_queue.h **/
extern volatile queue_t dead_pool_queue;
extern volatile queue_t rr_queue;
extern volatile queue_t system_queue;
extern volatile queue_t sleep_queue;

/* File scope variables **/
static volatile uint16_t kernel_sp;
static volatile uint8_t num_events_created = 0;
static volatile uint8_t is_async = 0;
static volatile uint8_t preempt_async = 0;

/**** Functions *******************************************************************/

/**
 * \fn kernel_main_loop
 *
 * \brief The heart of the RTOS, the main loop where the kernel is entered and exited.
 *
 * The complete function is:
 *
 *  Loop
 *<ol><li>Select and dispatch a process to run</li>
 *<li>Exit the kernel (The loop is left and re-entered here.)</li>
 *<li>Handle the request from the process that was running.</li>
 *<li>End loop, go to 1.</li>
 *</ol>
 *
 */
void kernel_main_loop(void) {
	
	for (;;) {		
		// If in asynchronous mode skip dispatch this time, current task should
		// still be running.
		if(is_async) {
			is_async = 0;
		} else {
			kernel_dispatch();
		}
		//
		exit_kernel();		
		
		/* if this task makes a system call, or is interrupted,
		 * the thread of control will return to here. */
		kernel_handle_request();
	}
}

/**
 * @fn kernel_dispatch
 *
 * @brief The second part of the scheduler.
 *
 * Chooses the next task to run.
 *
 *		This code was originally written by Scott Craig, Justin Tanner
 *		and Matt Campbell.  Heavily altered by authors.
 *
 */
static void kernel_dispatch(void) {
	
	#ifdef INSTRUMENT
	TEST_HIGH(ORANGE);	//Dispatch time
	#endif
	
	// Deal with async signal pre-emption
	if(preempt_async) {
		if(cur_task->state == RUNNING) {
			if(cur_task->level == RR) {
				enqueue_front(&rr_queue,cur_task);
			}
			cur_task->state = READY;
			timer_pause(&cur_task->execution_timer);
		}
	}
	
	/* If the current state is RUNNING, then select it to run again.
	 * kernel_handle_request() has already determined it should be selected.
	 */
	if (cur_task->state != RUNNING || cur_task == idle_task) {
		if (system_queue.head != NULL) {
			cur_task = dequeue(&system_queue);
		} else if (current_periodic_task != NULL) {
			/* Keep running the current PERIODIC task. */
			cur_task = current_periodic_task;
		} else if (rr_queue.head != NULL) {
			cur_task = dequeue(&rr_queue);
		} else {
			/* No task available, so idle. */
			cur_task = idle_task;
		}
	
		timer_resume(&(cur_task->execution_timer));
		cur_task->state = RUNNING;
	}
	
	#ifdef INSTRUMENT
	TEST_LOW(ORANGE);	//Dispatch time
	#endif	
}

/**
 * @fn kernel_handle_request
 *
 *@brief The first part of the scheduler.
 *
 * Perform some action based on the system call or timer tick.
 * Perhaps place the current process in a ready or waiting queue.
 *
 * This code was originally written by Scott Craig, Justin Tanner
 * and Matt Campbell.  Heavily altered by authors.
 */
static void kernel_handle_request(void) {
	
	switch (kernel_request) {
	case NONE:
		/* Should not happen. */
		break;

	case TIMER_EXPIRED:
	
		#ifdef INSTRUMENT
		TEST_HIGH(YELLOW);	//Tick update time
		#endif
				
		kernel_update_ticker();
		
		/* Periodic tasks should never over run their WCET. */
		if (cur_task->level == PERIODIC) {
			if(has_execution_time_elapsed(cur_task)) {	// Is this right?  The timer has a ms resolution and wcet has tick resolution
				error_msg = ERR_RUN_3_PERIODIC_TOOK_TOO_LONG;
				OS_Abort();
			}
		}

		/* Round robin tasks get pre-empted after QUANTUM ms (2 ticks by default). */
		if (cur_task->level == RR && cur_task->state == RUNNING) {			
			
			if(has_execution_time_elapsed(cur_task)) {	// same thing
				
				cur_task->state = READY;
				timer_reset(&(cur_task->execution_timer));
				enqueue(&rr_queue, cur_task);
				
			}
		}
		
		#ifdef INSTRUMENT
		TEST_LOW(YELLOW);	//Tick update time
		#endif
		
		break;

	case TASK_CREATE:
		kernel_request_retval = kernel_create_task();

		/* Check if new task has higher priority, and that it wasn't an ISR
		 * making the request.
		 */
		if (kernel_request_retval) {
			/* If new task is SYSTEM and cur is not, then don't run old one */
			if (kernel_request_create_args.level == SYSTEM && cur_task->level
					!= SYSTEM) {
				cur_task->state = READY;
			}

			/* If cur is RR, it might be pre-empted by a new PERIODIC. */
			if (cur_task->level == RR && kernel_request_create_args.level
					== PERIODIC ) {
				cur_task->state = READY;
			}

			/* enqueue READY RR tasks. */
			if (cur_task->level == RR && cur_task->state == READY) {
				enqueue_front(&rr_queue, cur_task);
			}
		}
		
		#ifdef INSTRUMENT
		TEST_LOW(RED);	//Test task create
		#endif
		
		break;

	case TASK_TERMINATE:
		if (cur_task != idle_task) {
			/* If a periodic task terminates is should no longer be the active periodic task. */
			if(cur_task->level == PERIODIC) {
				current_periodic_task = NULL;		
			}
			kernel_terminate_task();
		}
		break;

	case TASK_NEXT:
		switch (cur_task->level) {
		case SYSTEM:
			enqueue(&system_queue, cur_task);
			break;

		case PERIODIC:
			/* Periodic tasks should never over run their WCET. */
			
			if(has_execution_time_elapsed(cur_task)) {
				error_msg = ERR_RUN_3_PERIODIC_TOOK_TOO_LONG;
				OS_Abort();
			}
			timer_reset(&(cur_task->execution_timer));
			current_periodic_task = NULL;
			break;

		case RR:
			timer_reset(&(cur_task->execution_timer));
			enqueue(&rr_queue, cur_task);
			break;

		default: /* idle_task */
			break;
		}

		cur_task->state = READY;
		break;
		
	case EVENT_INIT:
		kernel_request_event_ptr = NULL;
		if(num_events_created < MAXEVENT){
			kernel_request_event_ptr = (EVENT *)(uint16_t)(++num_events_created);
		} else {
			kernel_request_event_ptr = (EVENT *) (uint16_t) 0;
		}
		
		break;
	
	case EVENT_CLEAR:
		kernel_event_clear();
		break;

	case EVENT_WAIT:
		/* idle_task does not wait. */
		if (cur_task != idle_task) {
			kernel_event_wait();
		}
		break;
		
	case EVENT_WAIT_NEXT:
		if (cur_task != idle_task) {
			kernel_event_clear();
			kernel_event_wait();
		}
		break;

	case EVENT_SIGNAL:
		kernel_event_signal();
		break;
		
	case EVENT_ASYNC_SIGNAL:
		is_async = 1;
		kernel_event_signal();
		break;
	
	default:
		/* Should never happen */
		error_msg = ERR_RUN_8_RTOS_INTERNAL_ERROR;
		OS_Abort();
		break;
	}
	kernel_request = NONE;
}

/**
 * @brief Update the current time and determine if preemption is necessary.
 *
 *		This code was originally written by Scott Craig, Justin Tanner
 *		and Matt Campbell.  Heavily altered by authors.
 */
static void kernel_update_ticker(void) {
	
	/* update the tick count each time we enter this function (timed interrupt fires once/tick) */
	cur_ticks++;
	if(cur_ticks == 0xFFFF) tick_overflow_flag = 1;
	
	uint8_t make_ready = 0;

	/* Check sleeping tasks */
	if (sleep_queue.head != NULL) {
		--(sleep_queue.head->diff_ticks_remaining);
		
		while (sleep_queue.head->diff_ticks_remaining == 0) {
			/* The waking task */
			task_descriptor_t* task_ptr = sleep_dequeue(&sleep_queue);

			task_ptr->state = READY;
			switch (task_ptr->level) {
			case SYSTEM:
				enqueue(&system_queue, task_ptr);
				break;
			case PERIODIC:
				//It is possible a periodic task has been terminated since it was added.
				if(task_ptr->state == DEAD) {
					break;
				}
				if(current_periodic_task != NULL) {
					//Conflicting periodic tasks!
					error_msg = ERR_RUN_9_PERIODIC_CONFLICT;
					OS_Abort();
				}
				current_periodic_task = task_ptr;
				kernal_request_sleep_ticks = current_periodic_task->period;
				// Enqueue for next period
				sleep_enqueue(&sleep_queue,task_ptr);
				break;
			case RR:
				enqueue(&rr_queue, task_ptr);
				break;
			default:
				break;
			}
			/* Check to see if current task needs to be pre-empted (never will have colision of PERIODIC & PERIODIC
					due to error check above) */
			if (cur_task != idle_task && !make_ready) {
				if (cur_task->level != SYSTEM && (task_ptr->level == SYSTEM || task_ptr->level == PERIODIC)) {
					make_ready = 1;
				}
			}
			if (sleep_queue.head == NULL) {
				break;
			}
		}
		if (make_ready && (cur_task != idle_task)) {
			cur_task->state = READY;
			// Periodic tasks should be left in current_periodic_task slot until they call next_task.
			if (cur_task->level == RR) {				
				if(has_execution_time_elapsed(cur_task)) {					
					timer_reset(&(cur_task->execution_timer));
					enqueue(&rr_queue, cur_task);
				} else {
					timer_pause(&(cur_task->execution_timer));
					enqueue_front(&rr_queue, cur_task);
				}
			}			
		}
	}
}

/**
 *  @brief Kernel function to create a new task.
 *
 * When creating a new task, it is important to initialize its stack just like
 * it has called "enter_kernel()"; so that when we switch to it later, we
 * can just restore its execution context on its stack.
 *
 *		This code was originally written by Scott Craig, Justin Tanner
 *		and Matt Campbell.  Altered by authors.
 * @sa enter_kernel
 */
int kernel_create_task(void) {
	/* The new task. */
	task_descriptor_t *p;
	uint8_t* stack_bottom;

	if (dead_pool_queue.head == NULL) {		
		return 0;
	}

	/* idling "task" goes in last descriptor. */
	if (kernel_request_create_args.level == NULL) {
		p = &task_desc[MAXPROCESS];
	}
	/* Find an unused descriptor. */
	else {
		p = dequeue(&dead_pool_queue);
	}

	stack_bottom = &(p->stack[MAXSTACK - 1]);

	/* The stack grows down in memory, so the stack pointer is going to end up
	 * pointing to the location 32 + 1 + 2 + 2 = 37 bytes above the bottom, to make
	 * room for (from bottom to top):
	 *   the address of Task_Terminate() to destroy the task if it ever returns,
	 *   the address of the start of the task to "return" to the first time it runs,
	 *   register 31,
	 *   the stored SREG, and
	 *   registers 30 to 0.
	 */
	uint8_t* stack_top = stack_bottom - STACK_CONTEXT_SIZE;
	
	// fill the registers with values for debug
	for( int i = 0; i < 31; i++ ){
		stack_top[i+1] = i;
	}

	/* stack_top[0] is the byte above the stack.
	 * stack_top[1] is r0. */
	stack_top[2] = (uint8_t) 0; /* r1 is the "zero" register. */
	/* stack_top[31] is r30. */
	stack_top[32] = (uint8_t) _BV(SREG_I); /* set SREG_I bit in stored SREG. */
	
	#ifdef MEGA_2560
		stack_top[33] = (uint8_t)0;	// EIND
		stack_top[34] = 31;			// r31
		
		stack_top[35] = (uint8_t)0;	// EIND
		stack_top[36] = (uint8_t) ((uint16_t) (kernel_request_create_args.f) >> 8);	
		stack_top[37] = (uint8_t) (uint16_t) (kernel_request_create_args.f);
		stack_top[38] = (uint8_t)0;	// EIND
		stack_top[39] = (uint8_t) ((uint16_t) Task_Terminate >> 8);
		stack_top[40] = (uint8_t) (uint16_t) Task_Terminate;
	#else
		stack_top[33] = 31;			// r31
		stack_top[34] = (uint8_t) ((uint16_t) (kernel_request_create_args.f) >> 8);
		stack_top[35] = (uint8_t) (uint16_t) (kernel_request_create_args.f);
		stack_top[36] = (uint8_t) ((uint16_t) Task_Terminate >> 8);
		stack_top[37] = (uint8_t) (uint16_t) Task_Terminate;
	#endif

	/*
	 * Make stack pointer point to cell above stack (the top).
	 * Make room for 32 registers, SREG and two return addresses.
	 */
	p->sp = stack_top;

	p->state = READY;
	p->arg = kernel_request_create_args.arg;
	p->level = kernel_request_create_args.level;
	p->wcet = kernel_request_create_args.wcet;
	p->period = kernel_request_create_args.period;
	
	timer_reset(&(p->execution_timer));

	switch (kernel_request_create_args.level) {
	case PERIODIC:
		/* Check if the task was created before the first roll over of the tick count. */
		if(tick_overflow_flag) {
			error_msg = ERR_RUN_11_PERIODIC_AFTER_ROLL_OVER;
			OS_Abort();
		}	
	
		/* Determine how long the periodic task needs to sleep before it is scheduled
			to run. */
		
		//Might need to start the task right away.		
		if(kernel_request_create_args.offset == 0) {
			if(current_periodic_task != NULL) {
				//Conflicting periodic tasks!
				error_msg = ERR_RUN_9_PERIODIC_CONFLICT;
				OS_Abort();
			}
			current_periodic_task = p;
			kernal_request_sleep_ticks = current_periodic_task->period;
		} else {
			kernal_request_sleep_ticks = kernel_request_create_args.offset;
		}
		
		// Enqueue for next period
		sleep_enqueue(&sleep_queue,p);
		break;
	case SYSTEM:
		/* Put SYSTEM and Round Robin tasks on a queue. */
		enqueue(&system_queue, p);
		break;

	case RR:
		/* Put SYSTEM and Round Robin tasks on a queue. */
		enqueue(&rr_queue, p);
		break;

	default:
		/* idle task does not go in a queue */
		break;
	}
	
	return 1;
}

/**
 * @brief Kernel function to destroy the current task.
 *		This code was originally written by Scott Craig, Justin Tanner
 *		and Matt Campbell.  Altered by authors.
 */
static void kernel_terminate_task(void) {
		/* deallocate all resources used by this task */
		cur_task->state = DEAD;
		
		if(cur_task->level == PERIODIC){	//remove from the sleep queue
			remove_from_sleep_queue(&sleep_queue, cur_task);
		}
		
		enqueue(&dead_pool_queue, cur_task);
}

/**** Events *************************************************************************/

/** @brief Clears previously unconsumed event signal */
static void kernel_event_clear(){
	uint16_t handle = ((uint16_t) (kernel_request_event_ptr) - 1);	
	events_state &= ~(1<<handle);
}

/** @brief Current task is waiting on signal. */
static void kernel_event_wait() {
	
	/* Check the handle of the event to ensure that it is initialized. */
	uint8_t handle = (uint8_t) ((uint16_t) (kernel_request_event_ptr) - 1);
	
	if (handle >= num_events_created) {
		/* Error code. */
		error_msg = ERR_RUN_5_WAIT_ON_BAD_EVENT;
		OS_Abort();
	} else if (cur_task->level == PERIODIC) {
		error_msg = ERR_RUN_7_PERIODIC_CALLED_WAIT;
		OS_Abort();
	} else {
		if(events_state & (1<<handle)){		// signal already set			
			events_state &= ~(1<<handle);	// clear old signal, task should continue
		} else if(events_waiting[handle] != NULL){
			error_msg = ERR_RUN_10_MULTIPLE_EVENTS_WAITING;
			OS_Abort();
		} else {
			/* Place this task in a queue. */
			cur_task->state = WAITING;
			events_waiting[handle] = cur_task;		
		}
	}
}

/**
 * @brief Kernel function to signal waiting processes.  If 
 * no task waiting flag will be set.
 */
static void kernel_event_signal() {
	
	uint8_t handle = (uint8_t) ((uint16_t) (kernel_request_event_ptr) - 1);
	
	if(handle >= num_events_created){
		/* Error code. */
		error_msg = ERR_RUN_4_SIGNAL_ON_BAD_EVENT;
		OS_Abort();
	}
	
	if(events_waiting[handle] != NULL){
		
		#ifdef INSTRUMENT
		TEST_HIGH(PURPLE);	//event wake time
		#endif
		
		task_descriptor_t *td = events_waiting[handle];
		
		events_waiting[handle] = NULL;
		
		td->state = READY;
		if(td->level == SYSTEM){
			enqueue(&system_queue,td);
		} else {
			enqueue(&rr_queue, td);
		}
		
		// Do we need to preempt anything? async preempts on next dispatch only.
		// Clear the flag during the next dispatch if set.
		if(is_async){
			if(cur_task->level != SYSTEM) {
				//Flag so that next dispatch we throw everything on the queues and let it sort them out.
				preempt_async = 1;
			}
		} else {
			//System tasks never get preempted
			if(cur_task->level != SYSTEM) {
				//Throw everything in the queues, let dispatch sort it out.
				//Periodic tasks get left in current periodic slot
				if(cur_task->level==RR) {
					enqueue_front(&rr_queue,cur_task);
				}
				timer_pause(&cur_task->execution_timer);
				cur_task->state=READY;
			}
		}		
	} else {
		events_state |= (1<<handle);
	}
	#ifdef INSTRUMENT
	TEST_LOW(BLUE);	//event signal time
	#endif
}

/** @brief Calculates whether a tasks execution time has elapsed */
static uint8_t has_execution_time_elapsed(task_descriptor_t *task){
	
	unsigned int t_val, worst_case;
	uint8_t sreg, r;

	sreg = SREG;
	cli();
	
	r = 0;
	t_val = timer_value(&(task->execution_timer));
	worst_case = task->wcet * TICK;
	
	if(task->level == RR) worst_case--;
	
	r = t_val >= worst_case;
	SREG = sreg;
	
	return r;
}

/**** Context switching *********************************************************************/
/**
 * It is important to keep the order of context saving and restoring exactly
 * in reverse. Also, when a new task is created, it is important to
 * initialize its "initial" context in the same order as a saved context.
 *
 * Save r31 and SREG on stack, disable interrupts, then save
 * the rest of the registers on the stack. In the locations this macro
 * is used, the interrupts need to be disabled, or they already are disabled.
 *
 * All code written by Scott Craig, Justin Tanner and Matt Campbell.
 */	

#ifdef MEGA_2560	// 2560
	#define    SAVE_CTX_TOP()       asm volatile (\
		"push   r31             \n\t"\
		"in		r31,0x3C		\n\t"\
		"push	r31				\n\t"\
		"in     r31,__SREG__    \n\t"\
		"cli                    \n\t"::); /* Disable interrupt */
#else			// XPlained
	#define    SAVE_CTX_TOP()       asm volatile (\
		"push   r31             \n\t"\
		"in     r31,__SREG__    \n\t"\
		"cli					\n\t"::); /* Disable interrupt */

#endif

#define STACK_SREG_SET_I_BIT()    asm volatile (\
    "ori    r31, 0x80        \n\t"::);

#define    SAVE_CTX_BOTTOM()       asm volatile (\
    "push   r31             \n\t"\
    "push   r30             \n\t"\
    "push   r29             \n\t"\
    "push   r28             \n\t"\
    "push   r27             \n\t"\
    "push   r26             \n\t"\
    "push   r25             \n\t"\
    "push   r24             \n\t"\
    "push   r23             \n\t"\
    "push   r22             \n\t"\
    "push   r21             \n\t"\
    "push   r20             \n\t"\
    "push   r19             \n\t"\
    "push   r18             \n\t"\
    "push   r17             \n\t"\
    "push   r16             \n\t"\
    "push   r15             \n\t"\
    "push   r14             \n\t"\
    "push   r13             \n\t"\
    "push   r12             \n\t"\
    "push   r11             \n\t"\
    "push   r10             \n\t"\
    "push   r9              \n\t"\
    "push   r8              \n\t"\
    "push   r7              \n\t"\
    "push   r6              \n\t"\
    "push   r5              \n\t"\
    "push   r4              \n\t"\
    "push   r3              \n\t"\
    "push   r2              \n\t"\
    "push   r1              \n\t"\
    "push   r0              \n\t"::);

/**
 * @brief Push all the registers and SREG onto the stack.
 */
#define    SAVE_CTX()    SAVE_CTX_TOP();SAVE_CTX_BOTTOM()

/**
 * @brief Pop all registers and the status register.
 */
#define    RESTORE_CTX_BOTTOM()    asm volatile (\
    "pop    r0                \n\t"\
    "pop    r1                \n\t"\
    "pop    r2                \n\t"\
    "pop    r3                \n\t"\
    "pop    r4                \n\t"\
    "pop    r5                \n\t"\
    "pop    r6                \n\t"\
    "pop    r7                \n\t"\
    "pop    r8                \n\t"\
    "pop    r9                \n\t"\
    "pop    r10             \n\t"\
    "pop    r11             \n\t"\
    "pop    r12             \n\t"\
    "pop    r13             \n\t"\
    "pop    r14             \n\t"\
    "pop    r15             \n\t"\
    "pop    r16             \n\t"\
    "pop    r17             \n\t"\
    "pop    r18             \n\t"\
    "pop    r19             \n\t"\
    "pop    r20             \n\t"\
    "pop    r21             \n\t"\
    "pop    r22             \n\t"\
    "pop    r23             \n\t"\
    "pop    r24             \n\t"\
    "pop    r25             \n\t"\
    "pop    r26             \n\t"\
    "pop    r27             \n\t"\
    "pop    r28             \n\t"\
    "pop    r29             \n\t"\
    "pop    r30             \n\t"::);

#ifdef MEGA_2560
	#define RESTORE_CTX_TOP() asm volatile (\
		"pop    r31             \n\t"\
		"out	__SREG__,r31		\n\t"\
		"pop	r31				\n\t"\
		"out    0x3C, r31    \n\t"\
		"pop    r31             \n\t"::);
#else
	#define RESTORE_CTX_TOP()    asm volatile (\
		"pop    r31             \n\t"\
		"out    __SREG__, r31    \n\t"\
		"pop    r31             \n\t"::);
#endif

#define    RESTORE_CTX()    RESTORE_CTX_BOTTOM();RESTORE_CTX_TOP()

/**
 * @fn exit_kernel
 *
 * @brief The actual context switching code begins here.
 *
 * This function is called by the kernel. Upon entry, we are using
 * the kernel stack, on top of which is the address of the instruction
 * after the call to exit_kernel().
 *
 * Assumption: Our kernel is executed with interrupts already disabled.
 *
 * The "naked" attribute prevents the compiler from adding instructions
 * to save and restore register values. It also prevents an
 * automatic return instruction.
 *
 * Code written by Scott Craig, Justin Tanner and Matt Campbell.
 */
static void exit_kernel(void) {
	#ifdef INSTRUMENT
	TEST_HIGH(BROWN);	//CTX switch time
	#endif
	
	/*
	 * The PC was pushed on the stack with the call to this function.
	 * Now push on the I/O registers and the SREG as well.
	 */
	SAVE_CTX();

	/*
	 * The last piece of the context is the SP. Save it to a variable.
	 */
	kernel_sp = SP;

	/*
	 * Now restore the task's context, SP first.
	 */
	SP = (uint16_t) (cur_task->sp);

	/*
	 * Now restore I/O and SREG registers.
	 */
	RESTORE_CTX();

	#ifdef INSTRUMENT
	TEST_LOW(BROWN);	//CTX switch time
	TEST_LOW(GREEN);	//Total kernel time
	#endif
	
	/*
	 * return explicitly required as we are "naked".
	 * Interrupts are enabled or disabled according to SREG
	 * recovered from stack, so we don't want to explicitly
	 * enable them here.
	 *
	 * The last piece of the context, the PC, is popped off the stack
	 * with the ret instruction.
	 */	
	asm volatile ("ret\n"::);
}

/**
 * @fn enter_kernel
 *
 * @brief All system calls eventually enter here.
 *
 * Assumption: We are still executing on cur_task's stack.
 * The return address of the caller of enter_kernel() is on the
 * top of the stack.
 *
 * Code written by Scott Craig, Justin Tanner and Matt Campbell.
 */
void enter_kernel(void) {
	
	#ifdef INSTRUMENT
	TEST_HIGH(GREEN);	//Total kernel time
	TEST_HIGH(BROWN);	//CTX switch time
	#endif
	
	/*
	 * The PC was pushed on the stack with the call to this function.
	 * Now push on the I/O registers and the SREG as well.
	 */
	SAVE_CTX();

	/*
	 * The last piece of the context is the SP. Save it to a variable.
	 */
	cur_task->sp = (uint8_t*) SP;

	/*
	 * Now restore the kernel's context, SP first.
	 */
	SP = kernel_sp;

	/*
	 * Now restore I/O and SREG registers.
	 */
	RESTORE_CTX();
	
	#ifdef INSTRUMENT
	TEST_LOW(BROWN);	//CTX switch time
	#endif

	/*
	 * return explicitly required as we are "naked".
	 *
	 * The last piece of the context, the PC, is popped off the stack
	 * with the ret instruction.
	 */
	asm volatile ("ret\n"::);
}

/**** Interrupts ****************************************/
/**
 *
 * @brief The interrupt handler for output compare interrupts on Timer 2
 *
 * Used to enter the kernel when a tick expires.
 *
 * Assumption: We are still executing on the cur_task stack.
 * The return address inside the current task code is on the top of the stack.
 *
 * The "naked" attribute prevents the compiler from adding instructions
 * to save and restore register values. It also prevents an
 * automatic return instruction.
 *
 *		This code was originally written by Scott Craig, Justin Tanner
 *		and Matt Campbell.  Heavily altered by authors.
 */
void TIMER_COMPARE_VEC(void) {
	#ifdef INSTRUMENT
	TEST_HIGH(GREEN);	//Total kernel time
	TEST_HIGH(BROWN);	//CTX switch time
	#endif
	/*
	 * Save the interrupted task's context on its stack,
	 * and save the stack pointer.
	 *
	 * On the cur_task's stack, the registers and SREG are
	 * saved in the right order, but we have to modify the stored value
	 * of SREG. We know it should have interrupts enabled because this
	 * ISR was able to execute, but it has interrupts disabled because
	 * it was stored while this ISR was executing. So we set the bit (I = bit 7)
	 * in the stored value.
	 */
	
	SAVE_CTX_TOP();

	STACK_SREG_SET_I_BIT();

	SAVE_CTX_BOTTOM();

	cur_task->sp = (uint8_t*) SP;

	/*
	 * Now that we already saved a copy of the stack pointer
	 * for every context including the kernel, we can move to
	 * the kernel stack and use it. We will restore it again later.
	 */
	SP = kernel_sp;
		
	/*
	 * Inform the kernel that this task was interrupted.
	 */
	kernel_request = TIMER_EXPIRED;

	/*
	 * Prepare for next tick interrupt.
	 */
	last_timer_value += TICK_COUNTER;
	OCRnX__ += TICK_COUNTER;

	/*
	 * Restore the kernel context. (The stack pointer is restored again.)
	 */
	SP = kernel_sp;

	/*
	 * Now restore I/O and SREG registers.
	 */
	RESTORE_CTX();
	
	#ifdef INSTRUMENT
	TEST_LOW(BROWN);	//CTX switch time
	#endif

	/*
	 * We use "ret" here, not "reti", because we do not want to
	 * enable interrupts inside the kernel.
	 * Explilictly required as we are "naked".
	 *
	 * The last piece of the context, the PC, is popped off the stack
	 * with the ret instruction.
	 */
	asm volatile ("ret\n"::);
}