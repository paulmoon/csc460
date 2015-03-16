/**
 * \file	os.c
 * \brief	An interface to access the kernel.
 *			Implements all functionality as defined in os.h
 *
 *			The design of this operating system is heavily inspired
 *			by the RTOS written by Scott Craig, Justin Tanner and Matt Campbell.
 *			Some of there original code was used directly in this implementation,
 *			and will be cited in the function brief where appropriate.
 *	@date 07/13
 *
 *	@author	Gordon Meyer
 *	@author Daniel McIlvaney
 *	@author Cale McNulty
 */ 

#include "os.h"
#include "kernel.h"
#include "error_code.h"
#include "task_descriptor_queue.h"

#include <util/delay.h>

/**** Members *****************************************************/

/* Accessible variables from kernel.h **/
extern volatile unsigned int cur_ticks;
extern task_descriptor_t task_desc[MAXPROCESS + 1];
extern task_descriptor_t* cur_task;
extern task_descriptor_t* current_periodic_task;
extern volatile create_args_t kernel_request_create_args;
extern task_descriptor_t* idle_task;
extern volatile kernel_request_t kernel_request;
extern volatile int kernel_request_retval;
extern uint8_t volatile error_msg;
extern volatile EVENT* kernel_request_event_ptr;

/* Queues from task_descriptor_queue.h **/
extern volatile queue_t dead_pool_queue;
extern volatile queue_t rr_queue;
extern volatile queue_t system_queue;
extern volatile queue_t periodic_queue;
task_descriptor_t *events_waiting[MAXEVENT];

/**** Prototypes **************************************************/

/** @brief External main function to be run as the first system task */
extern int r_main(void);
static int Task_Create(void(*f)(void), int arg, unsigned int level, unsigned int wcet,
					unsigned int period, unsigned int start);

/**** Functions ***************************************************/
/**
 *  @brief The idle task does nothing but busy loop.
 */
static void idle(void) {
	for (;;);
}

/**** Header Implamentations **************************************/

/** 
* @brief Initialize the operating system.  This is the first function called.
*		 It creates a system task from r_main.
*
*		Segments of this code originally written by Scott Craig, Justin Tanner and Matt Campbell.
*/
void OS_Init(){	
	
	// initialize event queue
	for(uint8_t i = 0; i < MAXEVENT; i++){
		events_waiting[i] = NULL;
	}
	
	
	// initialize all tasks but idle to be dead
	for(uint8_t i = 0; i < MAXPROCESS - 1; i++){
		task_desc[i].state = DEAD;
		task_desc[i].next = &task_desc[i + 1];
	}
	task_desc[MAXPROCESS - 1].next = NULL;
	
	dead_pool_queue.head = &task_desc[0];
	dead_pool_queue.tail = &task_desc[MAXPROCESS - 1];
	
	/* Create idle "task" */
	kernel_request_create_args.f = (voidfuncvoid_ptr) idle;
	kernel_request_create_args.level = NULL;
	kernel_create_task();
	
	/* Create "main" task as SYSTEM level. */
	kernel_request_create_args.f = (voidfuncvoid_ptr) r_main;
	kernel_request_create_args.level = SYSTEM;
	kernel_create_task();
	
	/* First time through. Select "main" task to run first. */
	cur_task = task_desc;
	cur_task->state = RUNNING;
	dequeue(&system_queue);
	
	cur_ticks = 0;
	tick_overflow_flag = 0;
	
	// enable Timer2 CompA interrupt
	TIMSK__ |= (1<<OCIEnX__);
	
	// set initial compare value
	OCRnX__ = TICK_COUNTER;
	
	// set timer prescaler 
	TCCRnB__ &= ~(PRESCALER_ZEROES);
	TCCRnB__ |= PRESCALER_ONES;	
	
	last_timer_value = 0;
	
	kernel_main_loop();
	
}

/** Abort the execution of this RTOS due to an unrecoverable error.
*		Segments of this code originally written by Scott Craig, Justin Tanner and Matt Campbell.
*/
void OS_Abort(){
	
	uint8_t i, j;
	uint8_t flashes;

	cli();

	/* Initialize port for output */
	LED_DDR = LED_MASK;
	
	flashes = error_msg + 1;

	for (;;) {
		LED_PORT = (uint8_t) (LED_MASK);

		for (i = 0; i < 100; ++i) {
			_delay_ms(25);
		}

		LED_PORT = (uint8_t) 0;

		for (i = 0; i < 40; ++i) {
			_delay_ms(25);
		}

		for (j = 0; j < flashes; ++j) {
			LED_PORT = LED_MASK;

			for (i = 0; i < 10; ++i) {
				_delay_ms(25);
			}

			LED_PORT = (uint8_t) 0;

			for (i = 0; i < 10; ++i) {
				_delay_ms(25);
			}
		}

		for (i = 0; i < 20; ++i) {
			_delay_ms(25);
		}
	}
}

/** Generic function to create RR, Periodic or System task.
*	@param f		Task function pointer.
*	@param arg		Task argument.
*	@param level	Task priority level. SYSTEM, PERIODIC, RR
*	@param wcet		Task worst case execution time.  NULL if task not Periodic.
*	@param period	The Periodic task's period.  RR Quantum.  NULL for System.
*	@param start	Periodic task start time.  NULL for RR and System.
*
*	@return			True if creation successful, False otherwise.
*		Segments of this code originally written by Scott Craig, Justin Tanner and Matt Campbell.
*/
static int Task_Create(void(*f)(void), int arg, unsigned int level, unsigned int wcet,
						unsigned int period, unsigned int start) {
							
	#ifdef INSTRUMENT
	TEST_HIGH(RED);	//Test task create
	#endif						
	
	int retval;
	uint8_t sreg;

	sreg = SREG;
	cli();
	
	kernel_request_create_args.f = (voidfuncvoid_ptr) f;
	kernel_request_create_args.arg = arg;
	kernel_request_create_args.level = level;
	kernel_request_create_args.wcet = wcet;
	kernel_request_create_args.period = period;
		
	/* If we have already skipped past the requested start time need to figure out the next start time. */
	if(level == PERIODIC){
		if (period == 0) {
			// Can't create a periodic task with period 0.
			error_msg = ERR_RUN_9_PERIODIC_CONFLICT;
			OS_Abort();
		}
		// Allow periodic tasks to be created later than the 1st tick.
		while(start < cur_ticks) {
			start += period;
		}
	}
	
	kernel_request_create_args.offset = start - cur_ticks;
	
	kernel_request = TASK_CREATE;
	enter_kernel();
	
	retval = kernel_request_retval;
	SREG = sreg;

	return retval;
}

/** Create a System task.
*	@param f		Task function pointer.
*	@param arg		Task argument.
*
*	@return			True if creation successful, False otherwise.
*/
int Task_Create_System(void (*f)(void), int arg){
	
	return Task_Create(f,arg,SYSTEM, NULL, NULL, NULL);
}

/** Create a Round Robin task.
*	@param f		Task function pointer.
*	@param arg		Task argument.
*
*	@return			True if creation successful, False otherwise.
*/
int Task_Create_RR(void (*f)(void), int arg){
	
	return Task_Create(f, arg, RR, (QUANTUM/TICK), NULL, NULL);
}

/** Create periodic task
*	@param f		Task function pointer.
*	@param arg		Task argument.
*	@param wcet		Task worst case execution time.
*	@param period	The Periodic task's period.
*	@param start	Periodic task start time.
*
*	@return			True if creation successful, False otherwise.
*/
int Task_Create_Period(void (*f)(void), int arg, unsigned int period, unsigned int wcet, unsigned int start){
	
	return Task_Create(f,arg,PERIODIC,wcet,period,start);
}

/** @brief Terminate the current task
*/
void Task_Terminate(){
	
	uint8_t sreg;

	sreg = SREG;
	cli();

	kernel_request = TASK_TERMINATE;
	enter_kernel();

	SREG = sreg;
}

/** @brief Move on to the next task
*/
void Task_Next(){
	
	uint8_t volatile sreg;

	sreg = SREG;
	cli();

	kernel_request = TASK_NEXT;
	enter_kernel();

	SREG = sreg;
}

/** @brief Get the argument for the current task
*	@return The value of the tasks argument.
*/
int Task_GetArg(){
	
	return cur_task->arg;
}

/**** Events *******************************************************/

/** @brief Initialize a new task.
*		Segments of this code originally written by Scott Craig, Justin Tanner and Matt Campbell.
* @return	Pointer to the newly created task.
*/
EVENT *Event_Init(){
	
	EVENT* event_ptr;
	uint8_t sreg;

	sreg = SREG;
	cli();

	kernel_request = EVENT_INIT;
	enter_kernel();

	event_ptr = (EVENT *) kernel_request_event_ptr;

	SREG = sreg;

	return event_ptr;
}

/** @brief Generic kernel event request.
*	@param e		Event pointer.
*	@param request	The specific request for the kernel.
*/
inline static void Event_Request(EVENT *e, kernel_request_t request){
	
	uint8_t sreg;

	sreg = SREG;
	cli();

	kernel_request = request;
	kernel_request_event_ptr = e;
	enter_kernel();
	
	SREG = sreg;
}

/** @brief Clear previously signaled events
*	@param e	Pointer to the event being cleared.
*/
void Event_Clear(EVENT *e){
	
	Event_Request(e, EVENT_CLEAR);
}

/** @brief Block until the event is signaled.  No block if previously signaled
*	@param e	Pointer to the event we are waiting on.
*/
void Event_Wait(EVENT *e){
	
	Event_Request(e, EVENT_WAIT);

	#ifdef INSTRUMENT
	TEST_LOW(PURPLE);	//event wake time
	#endif
}

/** @brief Block until the next event is signaled.
*	@param e	Pointer to the event we are waiting on.
*/
void Event_Wait_Next(EVENT *e){
	
	Event_Request(e, EVENT_WAIT_NEXT);	
	
	#ifdef INSTRUMENT
	TEST_LOW(PURPLE);	//event wake time
	#endif
}

/** @brief Signal event.  Waiting events will be added to their respective queue.
*	@param e	Pointer to the event we are signaling.
*/
void Event_Signal(EVENT *e){
	
	#ifdef INSTRUMENT
	TEST_HIGH(BLUE);	//event signal time
	#endif
	Event_Request(e, EVENT_SIGNAL);	
}

/** @brief Signal asynchronous event.  Waiting events will be added to their respective queue.
*	@param e	Pointer to the event we are signaling.
*/
void Event_Async_Signal(EVENT *e){
	
	#ifdef INSTRUMENT
	TEST_HIGH(BLUE);	//event signal time
	#endif
	Event_Request(e, EVENT_ASYNC_SIGNAL);	
}

/** @brief Get the current system time.
*	@return Number of ms since the RTOS started, or since last overflow (every 65.535 seconds).
*/
unsigned int Now(){
	
	uint8_t sreg;

	sreg = SREG;
	cli();
	
	uint8_t tcnt = TCNT2;
	uint8_t delta = (tcnt - last_timer_value);
	delta /= 43;
	unsigned int r = cur_ticks * TICK;
	r += (unsigned int)delta;
	
	SREG = sreg;
	
	return r;
}

/** @brief Dummy Main, The application entry point.
*	This function calls OS_Init and never returns.  The specification
*	stated that the crt.s file was to be overridden; however, Neal
*	(course TA) announced this would no longer be required.
*/
int main(void){	
// 	DDRB |= (1<<PB7);
// 	for(;;) {
// 		PORTB ^= (1<<PB7);
// 		_delay_ms(200);
// 	}
	
	
	OS_Init();	
	/*Usb_UART_Init();
	Sonar_Init();
	
	sei();
	
	for (;;){
		Sonar_Trigger();
		_delay_ms(500);
		uint16_t r = Sonar_Get_Distance();
		Usb_UART_Send_Byte(0xff);
		Usb_UART_Send_Byte(r>>8);
		Usb_UART_Send_Byte(r);
		Usb_UART_Send_Byte(0xff);
		_delay_ms(500);
	}*/
	return 0;
}