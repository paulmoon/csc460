#ifdef USE_TEST_006

/*
Desired trace:
T006;0;0;1;1;2;4;3;5;6;4;5;6;4;5;6;2;4;3;5;6;4;5;6;...    
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
    uint16_t v = Task_GetArg();    
    for(;;){
        add_to_trace(v);
        if( is_trace_full() && v == 2){
            Task_Create_System(dump_trace,0);
            Task_Terminate();
        }else if( is_trace_full()){
            Task_Terminate();
        }
        Task_Next();
    }
}

void s1(){
    uint16_t v = Task_GetArg();
    add_to_trace(v);
    Task_Create_RR(r,6);    
    add_to_trace(v);
    Task_Terminate();
}

void r(){
    uint16_t v = Task_GetArg();
    int i = 10;
    while( i > 0)
        add_to_trace(v);
        --i;
        _delay_ms(25);
        Task_Next();
    }
}

extern int r_main(){    
    uart_init();
    set_trace_test(6);

    add_to_trace(0);
    
    Task_Create_System(s1,1);
    Task_Create_Periodic(p1,2,5,2,0);
    Task_Create_Periodic(p1,3,5,2,1);
    Task_Create_RR(r,4);
    Task_Create_RR(r,5);

    add_to_trace(0);

    Task_Terminate();
    return 0;
}

#endif