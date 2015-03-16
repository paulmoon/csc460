/**
* @file   
*
* @brief Operating systems kernel functionality and data structures.
*
*		Some of this code was originally written by Scott Craig, Justin Tanner
*		and Matt Campbell.
*
*	@date 07/13
*
*	@author	Gordon Meyer
*	@author Daniel McIlvaney
*	@author Cale McNulty
*/

#ifndef __KERNEL_H__
#define __KERNEL_H__

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "error_code.h"
#include "os.h"
#include "timer.h"

#if !defined MEGA_2560 && !defined XPLAINED
	#error "Definition < MEGA_2560 | XPLAINED > not found."
#endif


/***** Instrumentation for finding timing information about the RTOS ***************/
//#define INSTRUMENT
/*	BLACK: Capture trigger
	BROWN: Context switch
	RED: Task create
	ORANGE: Dispatch
	YELLOW: Tick update
	GREEN: Full kernel time
	BLUE: Signal event
	PURPLE: Wake event
*/

/** The maximum number of names. Currently the same as the number of tasks. */
#define MAXNAME		MAXPROCESS

#ifdef MEGA_2560	// 2560
	/** The timer count for 5ms with a 16bit timer @8 prescaler with F_CPU= 16000000 */
	#define	TICK_COUNTER		10000	
	#define TIMSK__				TIMSK5
	#define OCIEnX__			OCIE5A
	#define OCRnX__				OCR5A
	#define TCCRnB__			TCCR5B
	#define PRESCALER_ZEROES	((1<<CS50) | (1<<CS52))
	#define PRESCALER_ONES		(1<<CS51)
	
	#define LED_MASK			_BV(PB7)	// L on the Arduino
	
#else			// Xplained
	/** The timer count for 5ms with a 8bit timer @256 prescaler with F_CPU= 11059200 */
	#define	TICK_COUNTER		216
	#define TIMSK__				TIMSK2
	#define OCIEnX__			OCIE2A
	#define OCRnX__				OCR2A
	#define TCCRnB__			TCCR2B
	#define PRESCALER_ZEROES	(1<<CS20)
	#define PRESCALER_ONES		(1<<CS21) | (1<<CS22)
	
	#define LED_MASK			_BV(PB2)	// LED3 on xplained
#endif	

/** The timer count for 1ms. */
#define MS_COUNTER (TICK_COUNTER / TICK)

/** LEDs for OS_Abort() */
#define LED_DDR			DDRB
#define LED_PORT		PORTB


/* Typedefs and data structures. */

typedef void (*voidfuncvoid_ptr)(void); /* pointer to void f(void) */

/**
 * @brief This is the set of states that a task can be in at any given time.
 */
typedef enum {
	DEAD = 0, RUNNING, READY, WAITING, SLEEPING
} task_state_t;

/**
 * @brief This is the set of kernel requests, i.e., a request code for each system call.
 */
typedef enum {
	NONE = 0,
	TIMER_EXPIRED,
	TASK_CREATE,
	TASK_TERMINATE,
	TASK_NEXT,
	TASK_SLEEP,
	EVENT_INIT,
	EVENT_CLEAR,
	EVENT_WAIT,
	EVENT_WAIT_NEXT,
	EVENT_SIGNAL,
	EVENT_ASYNC_SIGNAL
} kernel_request_t;

/**
 * @brief The arguments required to create a task.
 */
typedef struct {
	/** The code the new task is to run.*/
	voidfuncvoid_ptr f;
	/** A new task may be created with an argument that it can retrieve later. */
	int arg;
	/** Priority of the new task: RR, PERIODIC, SYSTEM */
	uint8_t level;
	/** If the task is PERIODIC, this is its worse case execution time. */
	unsigned int wcet;
	/** If the task is PERIODIC, this is its period. */
	unsigned int period;
	/** If the new task is PERIODIC, this is the period offset. */
	unsigned int offset;
} create_args_t;

typedef struct td_struct task_descriptor_t;

/**
 * @brief All the data needed to describe the task, including its context.
 */
struct td_struct {
	/** The stack used by the task. SP points in here when task is RUNNING. */
	uint8_t stack[MAXSTACK];
	/** A variable to save the hardware SP into when the task is suspended. */
	uint8_t* volatile sp; /* stack pointer into the "workSpace" */
	/** PERIODIC tasks need a name in the PPP array. */
	//uint8_t name;
	/** The state of the task in this descriptor. */
	task_state_t state;
	/** The argument passed to Task_Create for this task. */
	int arg;
	/** The priority (type) of this task. */
	uint8_t level;
	/** A link to the next task descriptor in the queue holding this task. */
	task_descriptor_t* next;
	/** A link to the next task descriptor in the queue holding these sleeping tasks */
	task_descriptor_t* sleep_next;
	/** The number of ticks remaining after the previous item in the queue is released. If it is the first item in the queue it is the number of ticks until it is released. */
	unsigned int diff_ticks_remaining;
	/** The worst case execution time of a periodic task */
	unsigned int wcet;
	/** a periodic tasks period */
	unsigned int period;
	/** Timer to determine the tasks duration of execution */
	volatile OS_TIMER execution_timer;
};

/**** Accessible variables ************************************************/
/** This table contains all task descriptors, regardless of state, plus idler. */
task_descriptor_t task_desc[MAXPROCESS + 1];

/** An array of task descriptors waiting on events */
task_descriptor_t *events_waiting[MAXEVENT];

/** The current kernel request. */
volatile kernel_request_t kernel_request;

/** The arguments required to create a task */
volatile create_args_t kernel_request_create_args;

/** The task descriptor of the currently RUNNING task. */
task_descriptor_t* cur_task;

/** The descriptor for the currently scheduled periodic task. */
task_descriptor_t* current_periodic_task;

/** The special "idle task" at the end of the descriptors array. */
task_descriptor_t* idle_task;

/** Return value for Task_Create() request. */
volatile int kernel_request_retval;

/** How many ticks the RTOS has been running. */
volatile unsigned int cur_ticks;

/** Has the tick count (cur_ticks) overflowed. */
volatile uint8_t tick_overflow_flag;

/** Records the time of the last timing update. Allows for sub tick times to be calculated */
volatile uint8_t last_timer_value;

/** The specific error code that triggered OS_Abort */
uint8_t volatile error_msg;

/** Argument for adding sleeping tasks */
volatile unsigned int kernal_request_sleep_ticks;

/**** Events *************************************************************/
/** 16 events, each bit represents an event. If the bit is high an event was previously 
* signaled and has not yet been consumed  **/
volatile uint16_t events_state;

/** Used to pass the event into the kernel */
volatile EVENT* kernel_request_event_ptr;

/**** prototypes *********************************************************/
int kernel_create_task(void);
void kernel_main_loop(void);
void enter_kernel(void) __attribute((noinline, naked));

#endif