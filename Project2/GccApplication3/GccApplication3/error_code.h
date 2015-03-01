/**
 * @file   error_code.h
 *
 * @brief Error messages returned in OS_Abort().
 *        Green errors are initialization errors
 *        Red errors are runt time errors
 *
 * CSC 460/560 Real Time Operating Systems - Mantis Cheng
 *
 * @author Scott Craig
 * @author Justin Tanner
 */
#ifndef __ERROR_CODE_H__
#define __ERROR_CODE_H__

enum {

/** RED ERRORS -- Run time errors. */

/** User called OS_Abort() */
ERR_RUN_1_USER_CALLED_OS_ABORT,

/** ISR made a request that only tasks are allowed. */
ERR_RUN_2_ILLEGAL_ISR_KERNEL_REQUEST,

/** RTOS Internal error in handling request. */
ERR_RUN_3_RTOS_INTERNAL_ERROR,


/** Periodic Tasks runtime errors */

/** Too many tasks created. Only allowed MAXPROCESS at any time.*/
ERR_RUN_4_TOO_MANY_TASKS,

/** WCET is too long, or PERIOD is too short. take your pick. */
ERR_RUN_5_INVALID_WCET_AND_PERIOD,

/** Invalid start time for periodic task. Start time is either negative or
   it was trying to start in the past. */
ERR_RUN_6_INVALID_START_TIME,

/** PERIODIC task still running at end of time slot. */
ERR_RUN_7_PERIODIC_TOOK_TOO_LONG,

ERR_RUN_8_TWO_PERIODIC_TASKS_READY,

ERR_RUN_9_PERIODIC_TASK_TIME_CONFLICT,

ERR_RUN_10_NO_FREE_PERIODIC_BLOCK,


/** SERVICE runtime errors */

/** Trying to creat too many services */
ERR_RUN_11_TOO_MANY_SERVICES,

/** Periodic tasks should not be able to subscribe to a service */
ERR_RUN_12_PERIODIC_CANT_SUBSCRIBE,

/** Trying to use an invalid service descriptor.*/
ERR_RUN_13_INVALID_SERVICE_DESCRIPTOR,

};


#endif
