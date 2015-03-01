#ifdef USE_TEST_002
/*
Test workflow:
1) r_main() system task prints to trace 0 twice. Adds two RR tasks rr(1) and rr2(2). 
2) RR Queue: rr(1), rr2(2). rr(1) runs, printing out (1) and going back to the queue.
3) RR Queue: rr2(2), rr(1). rr2(2) runs. Adds 2, 2 to the trace, and creates rr(3). rr2(2) yields, going back to the queue.
4) RR Queue: rr(1), rr(3), rr2(2). rr(1) runs, adding 1 to the trace.
5) RR Queue: rr(3), rr2(2), rr(1). rr(3) runs, adding 3 to the trace.
6) RR Queue: rr2(2), rr(1), rr(3). rr2(2) runs. Adds 2, 2 to the trace, and creates rr(4). Terminates.
7) RR Queue: rr(1), rr(3), rr(4). From now on, [1, 3, 4] will be printed to the trace until it's full.

Desired trace:
    T002;0;0;1;2;2;1;3;2;2;1;3;4;1;3;4;1;3;4...;1;3;4;
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "os.h"
#include "uart/uart.h"
#include "trace/trace.h"

void dump_trace(){
    print_trace();
}

/**
    RR task which adds its argument to the trace.
    Various RR tasks will add to the trace as they run.
    i.e 1;3;4;1;3;4;    
*/
void rr(){    
    int16_t v = Task_GetArg();
    for(;;){
        add_to_trace(v);
        _delay_ms(5);

        if( is_trace_full() ){
            Task_Create_System(dump_trace,0);
            Task_Terminate();
        }
    }
    
}

/**
    A RR task which creates a couple of RR tasks
*/
void rr2(){
    add_to_trace(2);
    Task_Create_RR(rr,3);
    add_to_trace(2);
    Task_Next();
    
    add_to_trace(2);
    Task_Create_RR(rr,4);
    add_to_trace(2);

    Task_Terminate();    
}


extern int r_main(){    
    uart_init();
    set_trace_test(2);

    add_to_trace(0);
    Task_Create_RR(rr,1);
    Task_Create_RR(rr2,2);
    add_to_trace(0);

    Task_Terminate();
    return 0;
}

#endif