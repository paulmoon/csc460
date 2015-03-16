/**
 * \file   error_code.h
 *
 * \brief Error messages returned in OS_Abort().
 *
 *		  Altered by Gordon Meyer, Daniel McIlvaney and Cale McNulty
 *
 * CSC 460/560 Real Time Operating Systems - Mantis Cheng
 *
 * @author Scott Craig
 * @author Justin Tanner
 */
 
#ifndef __ERROR_CODE_H__
#define __ERROR_CODE_H__

enum {

	/** GREEN ERRORS -- Initialize time errors (LED3 on Xplained). */

	// None
	
	/** RED ERRORS -- Run time errors (LED2 on Xplained). */

	/** User called OS_Abort() */
	ERR_RUN_1_USER_CALLED_OS_ABORT = 0,

	/** Too many tasks created. Only allowed MAXPROCESS at any time.*/
	ERR_RUN_2_TOO_MANY_TASKS,

	/** PERIODIC task still running at end of time slot. */
	ERR_RUN_3_PERIODIC_TOOK_TOO_LONG,

	/** Signaling on a non-initialized event. */
	ERR_RUN_4_SIGNAL_ON_BAD_EVENT,

	/** Event_wait() on unitialized EVENT */
	ERR_RUN_5_WAIT_ON_BAD_EVENT,

	/** ISR made a request that only tasks are allowed. */
	ERR_RUN_6_ILLEGAL_ISR_KERNEL_REQUEST,

	/** PERIODIC tasks cannot wait. */
	ERR_RUN_7_PERIODIC_CALLED_WAIT,

	/** RTOS Internal error in handling request. */
	ERR_RUN_8_RTOS_INTERNAL_ERROR,
	
	/** Two periodic tasks trying to run at once or zero period */
	ERR_RUN_9_PERIODIC_CONFLICT,
	
	/** Two events waiting on single event */
	ERR_RUN_10_MULTIPLE_EVENTS_WAITING,
	
	/** Created a periodic task too late */
	ERR_RUN_11_PERIODIC_AFTER_ROLL_OVER,
	
	/** Invalid Line Following State */
	ERR_RUN_12_INVALID_LINE_FOL_STATE
};

#endif