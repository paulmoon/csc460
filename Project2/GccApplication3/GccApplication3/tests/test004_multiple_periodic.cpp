#ifdef USE_TEST_004

/*
Create two periodic tasks call them A and B.
Task A runs every 10 ticks with a start of 4
Task B runs every 5 ticks with a start of 2
Therefore the schedule we should see is
    Task A:   4      14        24        34
    Task B: 2   7 12    17  22    27  32


Desired trace:
T004:0;0;2;1;2;2;1;2;2;1;2;2;1;2;2;1;2
    given that ticks are 5ms interval    
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "os.h"
#include "uart/uart.h"
#include "trace/trace.h"
#include "../profiler.h"


void dump_trace(){
    print_trace();
}

void p1(){
    /* run for 5 iterations  */    
    int i = 0;
    for(i = 0;i < 5; ++i){        
        add_to_trace(1);                 
        Task_Next();        
    }

    Task_Create_RR(dump_trace,0);
    Task_Terminate();
}

void p2(){
    int i = 0;
    for(i = 0;i < 10; ++i){
        add_to_trace(2);
        Task_Next();
    }
    
    Task_Terminate();
}

extern int r_main(){    
    uart_init();
    set_trace_test(4);    

    add_to_trace(0);
    
    Task_Create_Periodic(p1,1,10,2,4);
    Task_Create_Periodic(p2,2,5,2,2);

    add_to_trace(0);

    Task_Terminate();
    return 0;
}

#endif