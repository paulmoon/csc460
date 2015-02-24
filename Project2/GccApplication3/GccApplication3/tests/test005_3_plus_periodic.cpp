#ifdef USE_TEST_005

/*
Desired trace:
    T006;0;0;1;2;3;4;2;2;3;2;2;4;2;3;2;2;3;2;42;1;2;3;2;2;4;...
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "os.h"
#include "uart/uart.h"
#include "trace/trace.h"
#include "../profiler.h"


int global_counter = 30;
int periodic_counter = 0;

void dump_trace(){
    Task_Next();
    print_trace();
}

void p(){
    uint16_t v = Task_GetArg();
    for(;;){
        add_to_trace(v);
        if(global_counter <= 0){
            break;
        }
        Task_Next();
    }

    periodic_counter -=1;
    if(periodic_counter == 0){
        Task_Create_System(dump_trace,0);   
    }
    Task_Terminate();
}


extern int r_main(){    
    uart_init();
    set_trace_test(3);    

    add_to_trace(0);
    
    Task_Create_Periodic(p,1,100,1,0);
    Task_Create_Periodic(p,2,10,1,1);
    Task_Create_Periodic(p,3,25,1,2);
    Task_Create_Periodic(p,4,40,1,3);

    periodic_counter = 0;

    add_to_trace(0);

    Task_Terminate();
    return 0;
}

#endif