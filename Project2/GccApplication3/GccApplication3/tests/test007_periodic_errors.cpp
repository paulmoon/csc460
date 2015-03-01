#ifdef USE_TEST_007

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "os.h"
#include "uart/uart.h"
#include "trace/trace.h"
#include "../profiler.h"
#include "../error_code.h"

void loop_forever() {
    for(;;){}
}

void p() {
    for(;;) {
        Task_Next();
    }
}

void sys1() {
    Task_Create_Periodic(p,0,10,5,0);
}

void p2() {
    for (;;) {
        Task_Create_System(loop_forever,0); 
    }    
}

void p3() {
    Task_Create_Periodic(p,0,10,5,0);
}

extern int r_main(){    
    uart_init();
    set_trace_test(7);

    uint8_t e = ERR_RUN_3_PERIODIC_TOOK_TOO_LONG;
    // uint8_t e = ERR_RUN_6_INVALID_WCET_AND_PERIOD;
    // uint8_t e = ERR_RUN_7_INVALID_START_TIME;
    // uint8_t e = ERR_RUN_9_TWO_PERIODIC_TASKS_READY;
    // uint8_t e = ERR_RUN_10_PERIODIC_TASK_TIME_CONFLICT;

    if (e == ERR_RUN_3_PERIODIC_TOOK_TOO_LONG) {
        Task_Create_Periodic(loop_forever,0,10,8,0);
    } else if (e == ERR_RUN_6_INVALID_WCET_AND_PERIOD) {
        Task_Create_Periodic(p,0,5,10,0);		
    } else if (e == ERR_RUN_7_INVALID_START_TIME) {
        _delay_ms(50);
        Task_Create_Periodic(p,0,5,1,0);
    } else if (e == ERR_RUN_9_TWO_PERIODIC_TASKS_READY) {
        // The next two tasks will conflict immediately upon creation, because of the same start time.
        // Task_Create_Periodic(p,0,10,5,0);
        // Task_Create_Periodic(p,0,10,1,0);

        // These will conflict on the second cycle (at tick 20).
        Task_Create_Periodic(p2,0,10,5,0);
        Task_Create_Periodic(p2,0,19,5,1);
		
		// We create a periodic task A
		// A creates a system task B
		// B preempts A, and places A back onto the periodic queue.
        // Since B loops forever, the period countdown timer will eventually enqueue 
        // A onto the periodic ready queue again, resulting in two A tasks.
        Task_Create_Periodic(p2,0,10,5,0);

    } else if (e == ERR_RUN_10_PERIODIC_TASK_TIME_CONFLICT) {
        // Conflict because first periodic task never ends and the second one is then scheduled.
        //Task_Create_Periodic(loop_forever,0,10,5,0);
        //Task_Create_Periodic(p,0,10,5,1);

        // p3 creates a periodic task which wants to run immediately. error.
        Task_Create_Periodic(p3,0,10,5,0);
    }
    
    Task_Terminate();
    return 0;
}

#endif