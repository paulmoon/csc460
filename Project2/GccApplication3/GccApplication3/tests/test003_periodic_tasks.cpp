#ifdef USE_TEST_003

/*
Desired trace:
T003:0;0;25;75;125;175;225;
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
    Task_Next();    
    print_trace();    
}

void p(){
    /* run for 5 iterations  */
    int i = 0;
    for(i = 0;i < 5; ++i){       
        add_to_trace(Now());
        Task_Next();
    }

    //print_trace();
    Task_Create_RR(dump_trace,0);
    Task_Terminate();
}


extern int r_main(){    
    uart_init();
    set_trace_test(3);

    add_to_trace(0);
    Task_Create_Periodic(p,1,10,2,5);
    add_to_trace(0);

    Task_Terminate();
    return 0;
}

#endif