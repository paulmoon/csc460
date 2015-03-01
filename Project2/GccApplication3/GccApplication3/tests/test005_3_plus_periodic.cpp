#ifdef USE_TEST_005

/*
test005: Test 4 periodic tasks.

P1: Print out 1. Period=100, start time=0
P2: Print out 2. Period=10,  start time=1
P3: Print out 3. Period=25,  start time=2
P4: Print out 4. Period=40,  start time=3

P1: 1
P2:  2          2          2          2          2          2
P3:   3                         3                         3
P4:    4                                        4

Desired trace (r_main() prints out the first two zeros):
    T005;0;0;1;2;3;4;2;2;3;2;2;4;2;3;2;2;3;2;4;2;1;2;3;2;2;4;3;2;2;2...

The periodic_counter variable signals that all peridic tasks are complete, and 
a RR task can safely be created to dump the trace.
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "os.h"
#include "uart/uart.h"
#include "trace/trace.h"
#include "../profiler.h"


int periodic_counter = 0;

void dump_trace(){
    Task_Next();    
    print_trace();
}

void p(){
    uint16_t v = Task_GetArg();
    periodic_counter += 1;

	int i = 0;
    for(i = 0;i < 20; ++i){
        add_to_trace(v);
        Task_Next();
    }

    periodic_counter -= 1;
    if(periodic_counter <= 0){
        Task_Create_RR(dump_trace,0);
    }
    Task_Terminate();
}


extern int r_main(){    
    uart_init();
    set_trace_test(5);

    add_to_trace(0);
    
    Task_Create_Periodic(p,1,100,1,0);
    Task_Create_Periodic(p,2,10,1,1);
    Task_Create_Periodic(p,3,25,1,2);
    Task_Create_Periodic(p,4,40,1,3);

    add_to_trace(0);

    Task_Terminate();
    return 0;
}

#endif