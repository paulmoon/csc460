#ifdef USE_TEST_007

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "os.h"
#include "uart/uart.h"
#include "trace/trace.h"
#include "../profiler.h"
#include "../error_code.h"

void loop_forever(){
    for(;;){}
}
void p(){
    for(;;){
        Task_Next();
    }
}

void sys1(){
    Task_Create_Periodic(p,0,10,5,0);
}
void p2(){
    for(;;){
        Task_Create_System(loop_forever); 
    }    
}

void p3(){
    Task_Create_Periodic(p,0,10,5,0);
}

extern int r_main(){    
    uart_init();
    set_trace_test(7);
    int8_t e = ERR_RUN_3_PERIODIC_TOOK_TOO_LONG;

    if( e == ERR_RUN_3_PERIODIC_TOOK_TOO_LONG){
        Task_Create_Periodic(loop_forever,0,10,8,0);

    }else if( e == ERR_RUN_6_INVALID_WCET_AND_PERIOD){
        Task_Create_Periodic(p,0,5,10,0);
        //Task_Create_Periodic(p,0,-10,5,0);
        //Task_Create_Periodic(p,0,10,-5,0);

    }else if( e == ERR_RUN_7_INVALID_START_TIME){
        Task_Create_Periodic(p,0,5,1,-1);

        // _delay_ms_(50);
        // Task_Create_Periodic(p,0,5,1,0);

    }else if ( e == ERR_RUN_9_TWO_PERIODIC_TASKS_READY){
        // conflict immediately on creation.
        Task_Create_Periodic(p,0,10,5,0);
        Task_Create_Periodic(p,0,10,1,0);


        // will conflict on the second iteration.
        // Task_Create_Periodic(p2,0,10,5,0);
        // Task_Create_Periodic(p2,0,19,5,1);

    }else if ( e == ERR_RUN_10_PERIODIC_TASK_TIME_CONFLICT){

        // conflict because first periodic task never ends and 
        // the second one then is scheduled.
        Task_Create_Periodic(loop_forever,0,10,5,0);
        Task_Create_Periodic(p,0,10,5,1);

        // A creates a periodic tasks.
        // A creatse a system task B.
        // B preempt A, and place A back onto the periodic queue
        // B creates another periodic task C with start 0.
        // this should error        
        // Task_Create_Periodic(p2,0,10,5,0);        


        // p3 creates a periodic task which wants to run immediately. error.
        //Task_Create_Periodic(p3,0,10,5,0);

    }else if( e == ERR_RUN_12_WCET_OR_PERIOD_IS_ZERO){
        Task_Create_Periodic(p,0,0,5,0);

        //Task_Create_Periodic(p,0,10,0,0);
    }    
    
    Task_Terminate();
    return 0;
}

#endif