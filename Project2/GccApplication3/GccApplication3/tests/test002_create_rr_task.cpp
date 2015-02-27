#ifdef USE_TEST_002
/*
    desired trace:
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
    RR task which adds to the trace.
    As the variuos rr tasks compute they will add to the trace.
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
    A RR task which creates a couple RR tasks
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