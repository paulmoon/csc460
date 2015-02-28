/**
 * @file os.c
 *
 * @brief A Real Time Operating System
 *
 * Our implementation of the operating system described by Mantis Cheng in os.h.
 *
 * @author Scott Craig
 * @author Justin Tanner
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "os.h"
#include "kernel.h"
#include "error_code.h"
#include "profiler.h"
#include "uart/uart.h"
#include "trace/trace.h"


/* Needed for memset */
/* #include <string.h> */

/** @brief main function provided by user application. The first task to run. */
extern int r_main();


/** The task descriptor of the currently RUNNING task. */
static task_descriptor_t* cur_task = NULL;

/** Since this is a "full-served" model, the kernel is executing using its own stack. */
static volatile uint16_t kernel_sp;

/** This table contains all task descriptors, regardless of state, plus idler. */
static task_descriptor_t task_desc[MAXPROCESS + 1];

/** The special "idle task" at the end of the descriptors array. */
static task_descriptor_t* idle_task = &task_desc[MAXPROCESS];

/** The current kernel request. */
static volatile kernel_request_t kernel_request;

/** Arguments for Task_Create() request. */
static volatile create_args_t kernel_request_create_args;

/** Return value for Task_Create() request. */
static volatile int kernel_request_retval;

/** Argument and return value for Service class of requests. */
static volatile SERVICE* kernel_request_service_ptr;
static volatile int16_t kernel_request_service_data;


/** Number of tasks created so far */
static queue_t dead_pool_queue;

/** The ready queue for RR tasks. Their scheduling is round-robin. */
static queue_t rr_queue;

/** The ready queue for SYSTEM tasks. Their scheduling is first come, first served. */
static queue_t system_queue;

/** The ready queue for periodic tasks. It is an error if this queue has more than 1 ready task.*/
static queue_t periodic_queue;

/** Service variables */
static service services[MAXSERVICE];
static uint8_t num_services = 0;

/** Array holding references to the periodid tasks which exist*/
static task_descriptor_t* periodic_tasks[MAXPROCESS];

/** Error message used in OS_Abort() */
static uint8_t volatile error_msg = ERR_RUN_1_USER_CALLED_OS_ABORT;

/** TICKs since boot */
static uint16_t volatile ticks_since_boot = 0;


/* Forward declarations */
/* kernel */
static void kernel_main_loop(void);
static void kernel_dispatch(void);
static void kernel_handle_request(void);

/* context switching */
static void exit_kernel(void) __attribute((noinline, naked));
static void enter_kernel(void) __attribute((noinline, naked));
extern "C" void TIMER1_COMPA_vect(void) __attribute__ ((signal, naked));


static int kernel_create_task();
static void kernel_terminate_task(void);

/* service api */
static void kernel_service_init();
static void kernel_service_subscribe();
static void kernel_service_publish();

/* queues */
static void enqueue(queue_t* queue_ptr, task_descriptor_t* task_to_add);
static void queue_push(queue_t* queue_ptr, task_descriptor_t* task_to_add);
static task_descriptor_t* dequeue(queue_t* queue_ptr);


static void kernel_update_ticker(void);
static void idle (void);
static void _delay_25ms(void);
static uint16_t _Now(void);

/*
 * FUNCTIONS
 */
/**
 *  @brief The idle task does nothing but busy loop.
 */
static void idle (void)
{
    for(;;)
    {};
}


/**
 * @fn kernel_main_loop
 *
 * @brief The heart of the RTOS, the main loop where the kernel is entered and exited.
 *
 * The complete function is:
 *
 *  Loop
 *<ol><li>Select and dispatch a process to run</li>
 *<li>Exit the kernel (The loop is left and re-entered here.)</li>
 *<li>Handle the request from the process that was running.</li>
 *<li>End loop, go to 1.</li>
 *</ol>
 */
static void kernel_main_loop(void)
{
    for(;;)
    {
        kernel_dispatch();

        exit_kernel();

        /* if this task makes a system call, or is interrupted,
         * the thread of control will return to here. */

        kernel_handle_request();
    }
}


/**
 * @fn kernel_dispatch
 *
 *@brief The second part of the scheduler.
 *
 * Chooses the next task to run.
 *
 */
static void kernel_dispatch(void)
{
    /* If the current state is RUNNING, then select it to run again.
     * kernel_handle_request() has already determined it should be selected.
     */

    if(cur_task->state != RUNNING || cur_task == idle_task)
    {
		if(system_queue.head != NULL)
        {
            cur_task = dequeue(&system_queue);
        }
        else if( periodic_queue.head != NULL)
        {            
            cur_task = dequeue(&periodic_queue);
        }
        else if(rr_queue.head != NULL)
        {
            cur_task = dequeue(&rr_queue);
        }
        else
        {
            /* No task available, so idle. */
            cur_task = idle_task;
        }

        cur_task->state = RUNNING;
    }
}


/**
 * @fn kernel_handle_request
 *
 *@brief The first part of the scheduler.
 *
 * Perform some action based on the system call or timer tick.
 * Perhaps place the current process in a ready or waitng queue.
 */
static void kernel_handle_request(void)
{
   switch(kernel_request)
    {
    case NONE:
        /* Should not happen. */
        break;

    case TIMER_EXPIRED:        
        kernel_update_ticker();

        /* Round robin tasks get pre-empted on every tick. 
            This is wronge it is premepted every quantum... which so
            happens to be one tick.
        */
        if(cur_task->level == RR && cur_task->state == RUNNING)
        {
            cur_task->state = READY;
            enqueue(&rr_queue, cur_task);
        }
        break;

    case TASK_CREATE:
        kernel_request_retval = kernel_create_task();
        
        /* Check if new task has higer priority, and that it wasn't an ISR
         * making the request.
            Current Level -> New Task Level
            system  -> system   = don't switch process
            system  -> periodic =  if the periodic task is created, and there
                                is already a task enqueued then this is an error,
                                else we don't need to do anything.
            system  -> RR       = don't switch process
            period  -> system   = system should run. periodic task involuntary 
                                yields the processor to the system task.It should
                                be placed back onto the periodic ready queue
                                so that once the system task finishes it will 
                                yield back to the periodic task. During this 
                                time we don't want to keep ticking down on the
                                WCET time.
            period  -> period   = don't switch. If we are tyring to run a periodic
                                task immediately then we error out.
            period  -> RR       = don't switch process.
            RR      -> system   = give the processor up to the system task.
            RR      -> period   = we give the processor to the periodic task iff
                                the period task wants to run RIGHT now.
            RR      -> RR       = don't swithc process.
         */
        if(kernel_request_retval)
        {

            if( cur_task->level == SYSTEM && kernel_request_create_args.level == PERIODIC)
            {
                if( periodic_queue.size >= 2){ 
                    error_msg = ERR_RUN_9_TWO_PERIODIC_TASKS_READY;                    
                    OS_Abort();
                }                
            }
            else if( cur_task->level == PERIODIC  && kernel_request_create_args.level == SYSTEM)
            {
                cur_task->state = READY;

                if( periodic_queue.size >= 1) {
                    error_msg = ERR_RUN_10_PERIODIC_TASK_TIME_CONFLICT;
                    OS_Abort();
                }
                
                /**
                    Watch out for cases in which the system task you create
                    takes a long time and another periodic task tries to get scheduled.
                    Because we have reenqueue the cur_task, the new periodic task 
                    will conflict and OS_Abort                    
                */
                enqueue(&periodic_queue,cur_task);

                /** 
                    Wait I don't think this case can ever be reached
                */
                // if( periodic_queue.size >= 2) {
                //     error_msg = ERR_RUN_10_PERIODIC_TASK_TIME_CONFLICT;
                //     OS_Abort();
                // }
            }
            else if(cur_task->level == PERIODIC && kernel_request_create_args.level == PERIODIC)
            {

                /** trying to run a new periodic task while running already 
                    running a periodic task */
                if( kernel_request_create_args.start == ticks_since_boot){
                    error_msg = ERR_RUN_10_PERIODIC_TASK_TIME_CONFLICT;
                    OS_Abort();
                }

                /** error because we are trying to create two periodic tasks
                    which should run on this tick
                    When will this case ever be triggered?? */
                if(periodic_queue.size >= 2)                
                {
                    error_msg = ERR_RUN_9_TWO_PERIODIC_TASKS_READY;
                    OS_Abort();
                }

            }
            else if( cur_task->level == RR && kernel_request_create_args.level == SYSTEM)
            {
                cur_task->state = READY;

                queue_push(&rr_queue,cur_task);
                //enqueue(&rr_queue, cur_task);
            }
            else if(cur_task->level == RR && kernel_request_create_args.level == PERIODIC)
            {
                /** the periodic task should prempt the RR task,only
                    if has requested to run RIGHT now */
                if( kernel_request_create_args.start == ticks_since_boot)
                {
                    cur_task->state = READY;
                    
                    queue_push(&rr_queue,cur_task);
                    //enqueue(&rr_queue, cur_task);
                }
            }

        }else{
            error_msg = ERR_RUN_2_TOO_MANY_TASKS;
            OS_Abort();
        }
        break;

    case TASK_TERMINATE:
		if(cur_task != idle_task)
		{			
        	kernel_terminate_task();
		}
        break;

    case TASK_NEXT:
		switch(cur_task->level)
		{
	    case SYSTEM:
	        enqueue(&system_queue, cur_task);
			break;

	    case PERIODIC:	        
            /** don't need to do anything, the periodic task has already been 
                removed from the periodic_queue, and we don't want to requeue
                until it's timer has expired. */            
            cur_task->state = WAITING;            
	        break;

	    case RR:
	        enqueue(&rr_queue, cur_task);
	        break;

	    default: /* idle_task */
			break;
		}

		cur_task->state = READY;
        break;

    case TASK_GET_ARG:
        /* Should not happen. Handled in task itself. */
        break;

    case SERVICE_INIT:
        /* Copied directly from .... */
        kernel_service_init();        
        break;

    case SERVICE_SUBSCRIBE:
        /* Should we check to make sure the idle task doesn't call subscribe */
        kernel_service_subscribe();
        break;

    case SERVICE_PUBLISH:
        kernel_service_publish();
        break;

    default:
        /* Should never happen */
        error_msg = ERR_RUN_5_RTOS_INTERNAL_ERROR;        
        OS_Abort();
        break;
    }

    kernel_request = NONE;
}


/*
 * Context switching
 */
/**
 * It is important to keep the order of context saving and restoring exactly
 * in reverse. Also, when a new task is created, it is important to
 * initialize its "initial" context in the same order as a saved context.
 *
 * Save r31 and SREG on stack, disable interrupts, then save
 * the rest of the registers on the stack. In the locations this macro
 * is used, the interrupts need to be disabled, or they already are disabled.
 */
#define    SAVE_CTX_TOP()       asm volatile (\
    "push   r31             \n\t"\
    "in     r31,0X3C        \n\t"\
    "push   r31             \n\t"\
    "in     r31,__SREG__    \n\t"\
    "cli                    \n\t"::); /* Disable interrupt */

#define STACK_SREG_SET_I_BIT()    asm volatile (\
    "ori    r31, 0x80        \n\t"::); /* preserve SREG,make interrupts on?*/

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
#define    SAVE_CTX()    SAVE_CTX_TOP();SAVE_CTX_BOTTOM();

/**
 * @brief Pop all registers and the status register.
 */
#define    RESTORE_CTX()    asm volatile (\
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
    "pop    r30             \n\t"\
    "pop    r31             \n\t"\
	"out    __SREG__, r31   \n\t"\
    "pop    r31             \n\t"\
    "out    0X3C, r31       \n\t"\
    "pop    r31             \n\t"::);


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
 */
static void exit_kernel(void)
{
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
    SP = (uint16_t)(cur_task->sp);

    /*
     * Now restore I/O and SREG registers.
     */
    RESTORE_CTX();

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
 */
static void enter_kernel(void)
{
    /*
     * The PC was pushed on the stack with the call to this function.
     * Now push on the I/O registers and the SREG as well.
     */
    SAVE_CTX();

    /*
     * The last piece of the context is the SP. Save it to a variable.
     */
    cur_task->sp = (uint8_t*)SP;

    /*
     * Now restore the kernel's context, SP first.
     */
    SP = kernel_sp;

    /*
     * Now restore I/O and SREG registers.
     */
    RESTORE_CTX();

    /*
     * return explicitly required as we are "naked".
     *
     * The last piece of the context, the PC, is popped off the stack
     * with the ret instruction.
     */
    asm volatile ("ret\n"::);
}


/**
 * @fn TIMER1_COMPA_vect
 *
 * @brief The interrupt handler for output compare interrupts on Timer 1
 *
 * Used to enter the kernel when a tick expires.
 *
 * Assumption: We are still executing on the cur_task stack.
 * The return address inside the current task code is on the top of the stack.
 *
 * The "naked" attribute prevents the compiler from adding instructions
 * to save and restore register values. It also prevents an
 * automatic return instruction.
 */
void TIMER1_COMPA_vect(void)
{
	//PORTB ^= _BV(PB7);		// Arduino LED	
    
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

    cur_task->sp = (uint8_t*)SP;

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
     * Restore the kernel context. (The stack pointer is restored again.)
     */
    SP = kernel_sp;

    /*
     * Now restore I/O and SREG registers.
     */
    RESTORE_CTX();

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


/*
 * Tasks Functions
 */
/**
 *  @brief Kernel function to create a new task.
 *
 * When creating a new task, it is important to initialize its stack just like
 * it has called "enter_kernel()"; so that when we switch to it later, we
 * can just restore its execution context on its stack.
 * @sa enter_kernel
 */
static int kernel_create_task()
{
    /* The new task. */
    task_descriptor_t *p;
    uint8_t* stack_bottom;


    if (dead_pool_queue.head == NULL)
    {
        /* Too many tasks! */
        return 0;
    }
 
	/* idling "task" goes in last descriptor. */
	if(kernel_request_create_args.level == NULL)
	{
		p = &task_desc[MAXPROCESS];
	}
	/* Find an unused descriptor. */
	else
	{
	    p = dequeue(&dead_pool_queue);
	}

    /** Do sanity checks if we are trying to create a periodic task*/
    if( kernel_request_create_args.level == PERIODIC){

        if(kernel_request_create_args.wcet <= 0 ||
         kernel_request_create_args.period <= 0 ||
         kernel_request_create_args.wcet >= kernel_request_create_args.period)
        {
            error_msg = ERR_RUN_6_INVALID_WCET_AND_PERIOD;
            OS_Abort();
        }
        
        if( kernel_request_create_args.start < ticks_since_boot ) {
            /** periodic task must start "some" time after the current boot time */
            error_msg = ERR_RUN_7_INVALID_START_TIME;
            OS_Abort();
        }
    }

    stack_bottom = &(p->stack[MAXSTACK-1]);

    /* The stack grows down in memory, so the stack pointer is going to end up
     * pointing to the location 32 + 1 + 1 +  3 + 3 = 40 bytes above the bottom, to make
     * room for (from bottom to top):
     *   the address of Task_Terminate() to destroy the task if it ever returns,
     *   the address of the start of the task to "return" to the first time it runs,
     *   register 31,     
     *   the stored EIND
     *   the stored SREG, and
     *   registers 30 to 0.
     */
    // uint8_t* stack_top = stack_bottom - (32 + 1 + 1 + 3 + 3);

     // + 3 + 3 instead of +2 + 2 becuase of extended address space
     uint8_t* stack_top = stack_bottom - (32 + 1 + 1 + 3 + 3);

    /* Not necessary to clear the task descriptor. */
    /* memset(p,0,sizeof(task_descriptor_t)); */

    /* stack_top[0] is the byte above the stack.
     * stack_top[1] is r0. */
    stack_top[2] = (uint8_t) 0; /* r1 is the "zero" register. */
    /* stack_top[31] is r30. */
    stack_top[32] = (uint8_t) _BV(SREG_I); /* set SREG_I bit in stored SREG. */
    /* stack_top[33] is EIND */
    /* stack_top[34] is r31. */

    /* We are placing the address (16-bit) of the functions
     * onto the stack in reverse byte order (least significant first, followed
     * by most significant).  This is because the "return" assembly instructions
     * (ret and reti) pop addresses off in BIG ENDIAN (most sig. first, least sig.
     * second), even though the AT90 is LITTLE ENDIAN machine.
     */

    // hmm what if the function is actually 3 bytes long..
    stack_top[35] = (uint8_t)(0x00);
    stack_top[36] = (uint8_t)((uint16_t)(kernel_request_create_args.f) >> 8);
    stack_top[37] = (uint8_t)(uint16_t)(kernel_request_create_args.f);

    stack_top[38] = (uint8_t)(0x00);
    stack_top[39] = (uint8_t)((uint16_t)Task_Terminate >> 8);
    stack_top[40] = (uint8_t)(uint16_t)Task_Terminate;

    /*
     * Make stack pointer point to cell above stack (the top).
     * Make room for 32 registers, SREG and two return addresses.
     */
    p->sp = stack_top;

    p->state = READY;
    p->arg = kernel_request_create_args.arg;
    p->level = kernel_request_create_args.level;
	
	switch(kernel_request_create_args.level){ 
    	case PERIODIC:{
            /* Set the appropriate variables for the periodic task */            
            p->start = kernel_request_create_args.start;
            p->wcet = kernel_request_create_args.wcet;      
            p->period = kernel_request_create_args.period;

            /* setup the counters for the periodic tasks.
                the counter should begin first be initialized to the number
                of ticks before it should start running */
    		p->counter = p->start - ticks_since_boot;
            p->wcet_counter = p->wcet;

            p->state = WAITING;
            
            /* try to find a free periodic task block */
            int candidate_index = -1;
            int k;
            for( k = 0;k < MAXPROCESS; ++k){
                if( periodic_tasks[k] == NULL){
                    candidate_index = k;
                    break;
                }
            }
            if( candidate_index == -1){
                error_msg = ERR_RUN_8_NO_FREE_PERIODIC_BLOCK;
                OS_Abort();
            }
            periodic_tasks[candidate_index] = p;


            /** this periodic task should run right now, so enqueue onto the 
                    ready queue */
            if( p->start == ticks_since_boot){                 
                p->state = READY;

                /* restart the period and wcet counters */
                p->counter = p->period;
                p->wcet_counter = p->wcet;
                enqueue(&periodic_queue, p);
            }

    		break;
		}
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
 */
static void kernel_terminate_task(void)
{
    /* deallocate all resources used by this task */
    cur_task->state = DEAD;
    if(cur_task->level == PERIODIC)
    {        
        /* remove the periodic task from the periodic_tasks array */
        for(int i = 0;i < MAXPROCESS; ++i){
            if( periodic_tasks[i] == cur_task){
                periodic_tasks[i] = NULL;
            }
        }        
    }
    enqueue(&dead_pool_queue, cur_task);
}


/**
 * @brief Creaate a new service handle.
 * service_ptrs are really just integer wrapped as a pointer. 
 */
static void kernel_service_init(void)
{
    kernel_request_service_ptr = (SERVICE *)((uint16_t)0);
    if(num_services < MAXSERVICE)
    {
        /* Pass a number back to the task, but pretend it is a pointer.
         * It is the index of the event_queue plus 1.
         * (0 is return value for failure.)
         */
        kernel_request_service_ptr = (SERVICE *)((uint16_t)(num_services + 1));
        ++num_services;
    }
    else
    {
        /* Should we error out instead of returning NULL? */
        kernel_request_service_ptr = (SERVICE *)((uint16_t)0);
    }
}

/**
* @brief Kernel function use to subscribe the current task.
* Places the current task into a "waiting" state and enqueues
* it onto the appropriate service queue.
*/
static void kernel_service_subscribe(void)
{
    uint8_t service_index = (uint8_t)((uint16_t)(kernel_request_service_ptr) - 1);

    if( service_index < 0 || service_index >= num_services)
    {
        error_msg = ERR_RUN_11_INVALID_SERVICE_DESCRIPTOR;
        OS_Abort();
    }
    else if( cur_task->level == PERIODIC)
    {
        error_msg = ERR_RUN_12_PERIODIC_CANT_SUBSCRIBE;
        OS_Abort();
    }
    else
    {        
        /* The task is now blocked waiting for the service to publish,
        therefore place it in the queue of the service it is subscribing to */
        cur_task->state = WAITING;
        enqueue(&(services[service_index].queue),cur_task);
    }
}


/**
 * @brief internal kernel function to publish to a service.
 * All tasks which are currently enqueded on this service, are dequeuded
 * from their wait queue ans placed on to their appropriate ready queues. 
 */
static void kernel_service_publish(void)
{    
    /* get the handle/index to the service struct array */
    uint8_t service_index = (uint8_t)((uint16_t)(kernel_request_service_ptr) - 1);
    if( service_index < 0 || service_index >= num_services)
    {
        error_msg = ERR_RUN_11_INVALID_SERVICE_DESCRIPTOR;
        OS_Abort();
    }
    
    /** record the data into the service structure */    
    services[service_index].data = (int16_t) kernel_request_service_data;

    /* For every task which is currently waiting for this service, 
        we wake them up and place them on their associated ready queues */
    task_descriptor_t* t = NULL;
    uint8_t will_be_prempted = 0;
    while( services[service_index].queue.size > 0 )
    {            
        t = dequeue(&(services[service_index].queue));
        t->state = READY;

        switch(t->level){
            case SYSTEM:                
                will_be_prempted = 1;    
                enqueue(&system_queue,t);
                break;

            case RR:
                enqueue(&rr_queue,t);
                break;

            case PERIODIC:
            default: 
                error_msg = ERR_RUN_5_RTOS_INTERNAL_ERROR;
                OS_Abort();
                /* Should never get here */
                break;
        }
    }    

    /* A system task was taken off the waiting queue, therefore at the end 
        of this kernel call the current task may have to yield the processor*/
    if( will_be_prempted)
    {
        if(cur_task->level == RR )
        {
            cur_task->state = READY;
            queue_push(&rr_queue,cur_task);
            //enqueue(&rr_queue,cur_task);
        }
        else if( cur_task->level == PERIODIC )
        {
            cur_task->state = READY;
            queue_push(&periodic_queue,cur_task);

            if( periodic_queue.size >= 2)
            {
                error_msg = ERR_RUN_9_TWO_PERIODIC_TASKS_READY;
                OS_Abort();
            }
        }
    }
    
}


/*
 * Queue manipulation.
 */

/**
 * @brief Add a task to the back of the queue.
 *
 * @param queue_ptr the queue to insert in
 * @param task_to_add the task descriptor to add
 */
static void enqueue(queue_t* queue_ptr, task_descriptor_t* task_to_add)
{
    task_to_add->next = NULL;

    if(queue_ptr->head == NULL)
    {
        /* empty queue */
        queue_ptr->head = task_to_add;
        queue_ptr->tail = task_to_add;
        queue_ptr->size = 1;
    }
    else
    {
        /* put task at the back of the queue */
        queue_ptr->tail->next = task_to_add;
        queue_ptr->tail = task_to_add;
        queue_ptr->size += 1;
    }
}


/**
 * @brief Add a task to the front of the queue.
 *
 * @param queue_ptr the queue to insert in
 * @param task_to_add the task descriptor to add
 */
static void queue_push(queue_t* queue_ptr, task_descriptor_t* task_to_add)
{    
    if( queue_ptr->head == NULL)
    {
        /* empty queue */
        queue_ptr->head = task_to_add;
        queue_ptr->tail = task_to_add;
        queue_ptr->size = 1;

        task_to_add->next = NULL;
    }
    else
    {
        /* make the task the new head and link to the prev head */
        task_to_add->next = queue_ptr->head;
        queue_ptr->head = task_to_add;        
        queue_ptr->size += 1;        
    }
}


/**
 * @brief Pops head of queue and returns it.
 *
 * @param queue_ptr the queue to pop
 * @return the popped task descriptor
 */
static task_descriptor_t* dequeue(queue_t* queue_ptr)
{
    task_descriptor_t* task_ptr = queue_ptr->head;

    if(queue_ptr->head != NULL)
    {
        queue_ptr->head = queue_ptr->head->next;
        task_ptr->next = NULL;
        queue_ptr->size -= 1;
    }

    return task_ptr;
}


/**
 * @brief Update the current time.
 *
 * Perhaps move to the next time slot of the PPP.
 */
static void kernel_update_ticker(void)
{
    /* PORTD ^= LED_D5_RED; */
	//PORTB ^= _BV(PB7);
    ++ticks_since_boot;

    if (cur_task != NULL && 
        cur_task->level == PERIODIC  && 
        cur_task->state == RUNNING) 
    {
        /* should we check to make sure the task is actually running */ 
        cur_task->wcet_counter -= 1;

        if (cur_task->wcet_counter <= 0) {
            error_msg = ERR_RUN_3_PERIODIC_TOOK_TOO_LONG;
            OS_Abort();
        }
    }    

    /** iterate through all the periodic tasks and decrement their counters 
        and enqueue any ready periodic tasks which tick down to 0 */
    for(int i = 0;i < MAXPROCESS; ++i){
        if( periodic_tasks[i] == NULL){continue;}

        /** decrment the timer counter */
        periodic_tasks[i]->counter -= 1;

        if( periodic_tasks[i]->counter <= 0){            
            /* set the period task to be in the ready queue */
            periodic_tasks[i]->state = READY;
            enqueue(&periodic_queue,periodic_tasks[i]);
            

            /* restart the counters  for the periodic tasks */
            periodic_tasks[i]->counter = periodic_tasks[i]->period;
            periodic_tasks[i]->wcet_counter = periodic_tasks[i]->wcet;
        }
    }    

    // check to see if there are more than two tasks in the periodic ready queue 
    if( periodic_queue.size >= 2)
    {                        
        error_msg = ERR_RUN_9_TWO_PERIODIC_TASKS_READY;
        OS_Abort();
    }

    /* current task is periodic and still running, yet we are trying
        to start running a new period task */
    if( cur_task != NULL && 
        cur_task->level == PERIODIC &&
        periodic_queue.size >= 1 )        
    {
        error_msg = ERR_RUN_10_PERIODIC_TASK_TIME_CONFLICT;
        OS_Abort();   
    }
    
}
		


#undef SLOW_CLOCK

#ifdef SLOW_CLOCK
/**
 * @brief For DEBUGGING to make the clock run slower
 *
 * Divide CLKI/O by 64 on timer 1 to run at 125 kHz  CS3[210] = 011
 * 1 MHz CS3[210] = 010
 */
static void kernel_slow_clock(void)
{
    TCCR1B &= ~(_BV(CS12) | _BV(CS10));
    TCCR1B |= (_BV(CS11));
}
#endif

/**
 * @brief Setup the RTOS and create main() as the first SYSTEM level task.
 *
 * Point of entry from the C runtime crt0.S.
 */
void OS_Init()
{
    int i;

    /* Set up the clocks */
	TCCR1A = 0;
	TCCR1B = (_BV(WGM32) | _BV(CS11));
	//TCCR1B = 0;	
    //TCCR1B |= (_BV(CS11));

#ifdef SLOW_CLOCK
    kernel_slow_clock();
#endif
    

    /* init the size of the queues to zero */
    dead_pool_queue.size = MAXPROCESS;
    rr_queue.size = 0;
    system_queue.size  = 0;
    periodic_queue.size = 0;

    num_services = 0;

    /*
     * Initialize dead pool to contain all but last task descriptor.
     *
     * DEAD == 0, already set in .init4
     */
    for (i = 0; i < MAXPROCESS - 1; i++)
    {
        task_desc[i].state = DEAD;
        periodic_tasks[i] = NULL;
        //name_to_task_ptr[i] = NULL;
        task_desc[i].next = &task_desc[i + 1];
    }
    periodic_tasks[MAXPROCESS-1] = NULL;
    task_desc[MAXPROCESS - 1].next = NULL;
    dead_pool_queue.head = &task_desc[0];
    dead_pool_queue.tail = &task_desc[MAXPROCESS - 1];

	/* Create idle "task" */
    kernel_request_create_args.f = (voidfuncvoid_ptr)idle;
    kernel_request_create_args.level = NULL;
    kernel_create_task();

    /* Create "main" task as SYSTEM level. */
    kernel_request_create_args.f = (voidfuncvoid_ptr)r_main;
    kernel_request_create_args.level = SYSTEM;
    kernel_create_task();

    /* First time through. Select "main" task to run first. */
    cur_task = task_desc;
    cur_task->state = RUNNING;
    dequeue(&system_queue);

    
    /* Set up Timer 1 Output Compare interrupt,the TICK clock. */
    TIMSK1 |= _BV(OCIE1A);
    /*OCR1A = TCNT1 + TICK_CYCLES;*/
    OCR1A = TICK_CYCLES;
    TCNT1 = 0;
    /* Clear flag. */
    TIFR1 = _BV(OCF1A);

    /*
     * The main loop of the RTOS kernel.
     */
    kernel_main_loop();
}


/**
 *  @brief Delay function adapted from <util/delay.h>
 */
static void _delay_25ms(void)
{
    //uint16_t i;

    /* 4 * 50000 CPU cycles = 25 ms */
    //asm volatile ("1: sbiw %0,1" "\n\tbrne 1b" : "=w" (i) : "0" (50000));
    _delay_ms(25);
}


/** @brief Abort the execution of this RTOS due to an unrecoverable erorr.
 */
void OS_Abort(void)
{
    uint8_t i, j;
    uint8_t flashes, mask;

    Disable_Interrupt();

    /* Initialize port for output */
    DDRB = LED_RED_MASK | LED_GREEN_MASK;

    if(error_msg < ERR_RUN_1_USER_CALLED_OS_ABORT)
    {
        flashes = error_msg + 1;
        mask = LED_GREEN_MASK;
    }
    else
    {
        flashes = error_msg + 1 - ERR_RUN_1_USER_CALLED_OS_ABORT;
        mask = LED_RED_MASK;
    }


    for(;;)
    {
        PORTB = (uint8_t)(LED_RED_MASK | LED_GREEN_MASK);

        for(i = 0; i < 100; ++i)
        {
               _delay_25ms();
        }

        PORTB = (uint8_t) 0;

        for(i = 0; i < 40; ++i)
        {
               _delay_25ms();
        }


        for(j = 0; j < flashes; ++j)
        {
            PORTB = mask;

            for(i = 0; i < 10; ++i)
            {
                _delay_25ms();
            }

            PORTB = (uint8_t) 0;

            for(i = 0; i < 10; ++i)
            {
                _delay_25ms();
            }
        }

        for(i = 0; i < 20; ++i)
        {
            _delay_25ms();
        }
    }
}

/**
 * @param f  a parameterless function to be created as a process instance
 * @param arg an integer argument to be assigned to this process instance
 * @return 0 if not successful; otherwise non-zero.
 * @sa Task_GetArg(), PPP[].
 *
 *  A new process  is created to execute the parameterless
 *  function @a f with an initial parameter @a arg, which is retrieved
 *  by a call to Task_GetArg().  If a new process cannot be
 *  created, 0 is returned; otherwise, it returns non-zero. 
 */
int8_t   Task_Create_System(void (*f)(void), int16_t arg){
	int retval;
	uint8_t sreg;

	sreg = SREG;
	Disable_Interrupt();

	kernel_request_create_args.f = (voidfuncvoid_ptr)f;
	kernel_request_create_args.arg = arg;
	kernel_request_create_args.level = (uint8_t)SYSTEM;	

	kernel_request = TASK_CREATE;
	enter_kernel();

	retval = kernel_request_retval;
	SREG = sreg;

	return retval;
}

/**
 * @param f  a parameterless function to be created as a process instance
 * @param arg an integer argument to be assigned to this process instance
 * @return 0 if not successful; otherwise non-zero.
 * @sa Task_GetArg(), PPP[].
 *
 *  A new process  is created to execute the parameterless
 *  function @a f with an initial parameter @a arg, which is retrieved
 *  by a call to Task_GetArg().  If a new process cannot be
 *  created, 0 is returned; otherwise, it returns non-zero. 
 */
int8_t   Task_Create_RR(    void (*f)(void), int16_t arg){
	int retval;
	uint8_t sreg;

	sreg = SREG;
	Disable_Interrupt();

	kernel_request_create_args.f = (voidfuncvoid_ptr)f;
	kernel_request_create_args.arg = arg;
	kernel_request_create_args.level = (uint8_t)RR;

	kernel_request = TASK_CREATE;
	enter_kernel();

	retval = kernel_request_retval;
	SREG = sreg;

	return retval;
}

/**
 * @param f  a parameterless function to be created as a process instance
 * @param arg an integer argument to be assigned to this process instance
 * @paarm period an integer defining the number of ticks before reshdeduling the task
 * @param wcet worst case estimated time (TICKS)
 * @param start the number of ticks since boot before first scheduling the task
 * @return 0 if not successful; otherwise non-zero.
 * @sa Task_GetArg(), PPP[].
 *
 *  A new process  is created to execute the parameterless
 *  function @a f with an initial parameter @a arg, which is retrieved
 *  by a call to Task_GetArg().  If a new process cannot be
 *  created, 0 is returned; otherwise, it returns non-zero. 
 */
int8_t   Task_Create_Periodic(void(*f)(void), int16_t arg, uint16_t period, uint16_t wcet, uint16_t start){
    int retval;
    uint8_t sreg;

    sreg = SREG;
    Disable_Interrupt();

    kernel_request_create_args.f = (voidfuncvoid_ptr)f;
    kernel_request_create_args.arg = arg;
    kernel_request_create_args.level = (uint8_t)PERIODIC;
	
	kernel_request_create_args.period = period;
	kernel_request_create_args.start = start;
	kernel_request_create_args.wcet = wcet;

	kernel_request = TASK_CREATE;
	enter_kernel();

    retval = kernel_request_retval;
    SREG = sreg;

    return retval;	
}


/**
  * @brief The calling task gives up its share of the processor voluntarily.
  */
void Task_Next()
{
    uint8_t volatile sreg;

    sreg = SREG;
    Disable_Interrupt();

    kernel_request = TASK_NEXT;
    enter_kernel();

    SREG = sreg;
}


/**
  * @brief The calling task terminates itself.
  */
void Task_Terminate()
{
    uint8_t sreg;

    sreg = SREG;
    Disable_Interrupt();

    kernel_request = TASK_TERMINATE;
    enter_kernel();

    SREG = sreg;
}


/** @brief Retrieve the assigned parameter.
 */
int Task_GetArg(void)
{
    int arg;
    uint8_t sreg;

    sreg = SREG;
    Disable_Interrupt();

    arg = cur_task->arg;

    SREG = sreg;

    return arg;
}

/**
 * @brief Create a new service descdriptor. 
 */
SERVICE *Service_Init(){
    SERVICE* service_ptr;
    uint8_t sreg;

    sreg = SREG;
    Disable_Interrupt();

    kernel_request = SERVICE_INIT;
    enter_kernel();

    service_ptr = (SERVICE *)kernel_request_service_ptr;

    SREG = sreg;
    return service_ptr;
}

/**
 * @brief. Current task subscribes to the service.
 * @param s the service to subscribe to.
 * @param v a pointer in which to place the published result.
 *
 * This will make two kernel context switches.
 * The first time is to block on the service until it publishes.
 * the second is to retrieve the data from the service publish. 
 */
void Service_Subscribe( SERVICE *s, int16_t *v ){
    uint8_t sreg;    

    sreg = SREG;
    Disable_Interrupt();

    kernel_request_service_ptr = (SERVICE*) s;
    kernel_request_service_data = (int16_t)0;
    kernel_request = SERVICE_SUBSCRIBE;
    
    //add_to_trace((uint16_t)s);
    //add_to_trace((uint16_t)cur_task);	

    enter_kernel();

    *v = services[((uint16_t)(s)-1)].data;

    SREG = sreg;
}

/**
* @brief Cause the service s to publish with value v
* @param s the service which will do the publshing
* @param v the int16_t to publish.
*
* If the current_task publishes and a higher-priority task gets 'un-blocked'
* then it will prempt the current task.
*/
void Service_Publish( SERVICE *s, int16_t v ){
    uint8_t sreg;

    sreg = SREG;
    Disable_Interrupt();

    kernel_request = SERVICE_PUBLISH;
    kernel_request_service_ptr = (SERVICE*) s;
    kernel_request_service_data = (int16_t) v;
    enter_kernel();

    SREG = sreg;
}



/**
	Now() has a resolution of milliseconds. 
	It will be able to accurately report up to 65536*TICK ms of time.
	Therefore given that we have a TICK resolution of 5ms, this function will be able
	to be used for timing tasks within the range of 0 to 327000ms.
*/
uint16_t Now() {	
    uint16_t ret_val;
    uint8_t sreg;

    sreg = SREG;
    Disable_Interrupt();

    ret_val = _Now();

    SREG = sreg;
	
    return ret_val; 
};

/* Pulled straight out of the arduino libary 
    http://arduino.cc/en/reference/map

static int16_t mapi(int16_t x, int16_t in_min, int16_t in_max, int16_t out_min, int16_t out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
*/

/**
* ticks_since_boot*TICK 
*     obtain the number of milliseconds counted by the TIMER1 ISR.        
* TCNT1 + (TICK_CYCLES/(TICK << 1)) 
*     we want the number of CPU ticks counted by the internal timer, but we need to do some rounding.
*     TICK_CYLCES/(TICK << 1) <==> (TICK_CYCLES/TICK)/2 
*     this round the number by 0.5 milliseconds, and therefore allows us avoid
*     case where we read a value of 4.999 ms but report 4.000 ms due to integer rounding.
* (TCNT1 + (TICK_CYCLES/(TICK << 1))*10)/TICK_CYCLES
*     We scale by 10 in order to avoid integer rounding when we divide by the TICK_CYCLES
*     ( i.e 50/100 --> 0 ). The final result will be a value between 0 and 10.
* mapi((TCNT1 + (TICK_CYCLES/(TICK << 1))*10)/TICK_CYCLES,0,10,0,TICK) <==>
*   ((((TCNT1 + half_tick_cycle)*10)/TICK_CYCLES)*TICK)/TICK_CYCLES
*     After doing the scale by 10 and division by TICK_CYCLES, we want to map the range
*     0 to 10 to the range 0 and TICK in order to obtain the number of milliseconds we can count
*     from the timer1 counter         
*/
uint16_t _Now(){
    static uint16_t half_tick_cycle = TICK_CYCLES/(TICK << 1);
    return ticks_since_boot*TICK + ((((TCNT1 + half_tick_cycle)*10)/TICK_CYCLES)*TICK)/TICK_CYCLES;
    //return ticks_since_boot*TICK + mapi( ((TCNT1 + half_tick_cycle)*10)/TICK_CYCLES,0,10,0,TICK);
}



/**
 * Runtime entry point into the program; just start the RTOS.  The application layer must define r_main() for its entry point.
*/
int main()
{	
	InitializeLogicAnalyzerProfiler();
	DisableProfileSample1();
	DisableProfileSample2();
	DisableProfileSample3();    
    // EnableProfileSample1();
    // EnableProfileSample2();
    // EnableProfileSample3();
	
	OS_Init();
	return 0;
}

