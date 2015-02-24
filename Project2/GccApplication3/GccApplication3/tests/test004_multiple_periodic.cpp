#ifdef USE_TEST_004

/*
Desired trace:
T004:0;0;2;2;1;2;2;1;2;2;1;2;2;1;2;2;1;2;2;
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
        //EnableProfileSample1();        
        add_to_trace(1); 
        //DisableProfileSample1();       
        
        Task_Next();        
    }

    Task_Create_System(dump_trace,0);
    Task_Terminate();
}

void p2(){
    int i = 0;
    for(i = 0;i < 10; ++i){
        //EnableProfileSample2();
        add_to_trace(2);
        //DisableProfileSample2();
        Task_Next();
    }
    
    Task_Terminate();
}

extern int r_main(){    
    uart_init();
    set_trace_test(3);    

    add_to_trace(0);
    
    Task_Create_Periodic(p1,1,10,2,0);
    Task_Create_Periodic(p2,2,5,2,0);

    add_to_trace(0);

    Task_Terminate();
    return 0;
}

#endif