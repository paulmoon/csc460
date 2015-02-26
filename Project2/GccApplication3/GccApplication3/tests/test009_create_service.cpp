#ifdef USE_TEST_009

/*
    Desired Trace:
    T010;0;0;1;1;2;2;3;3;2;2;3;3;3;3;3;3;...
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "os.h"
#include "uart/uart.h"
#include "trace/trace.h"
#include "../profiler.h"
#include "../error_code.h"

SERVICE* services[3];

void r(){
    add_to_trace(3);
    services[0] = Service_Init();
    add_to_trace(3);
    for(;;){
        add_to_trace(3);
        if( is_trace_full()){
            print_trace();
            Task_Terminate();
        }
        Task_Next();
    }    
}

void p(){
    add_to_trace(2);
    services[1] = Service_Init();
    add_to_trace(2);
    
    Task_Next();

    add_to_trace(2);
    services[1] = Service_Init();
    add_to_trace(2);

    Task_Terminate();
}

void s(){
    add_to_trace(1);
    services[2] = Service_Init();
    add_to_trace(1);
    Task_Terminate();
}

extern int r_main(){    
    uart_init();

    add_to_trace(0);
    Task_Create_RR(r,0);    
    Task_Create_Periodic(p,1,10,5,0);
    Task_Create_System(s,0);
    add_to_trace(0);
   
    Task_Terminate();
    return 0;
}

#endif