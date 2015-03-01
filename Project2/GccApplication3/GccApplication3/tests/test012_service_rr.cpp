#ifdef USE_TEST_012

/*
Testing RR tasks subscribing to another RR publisher.
    Desired Trace:
    T012;1;1;3;3;5;5;7;7;9;9;11;11;13;13;15;15;17;17;19;19;
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

int g_count = 0;
void r(){
    int16_t v;
    while( g_count < 20){
        Service_Subscribe(services[0], &v);
        add_to_trace(v);
        g_count += 1;
    }
}

void r2(){
    int16_t count = 0;
    for(;;){
        Service_Publish(services[0],count++);
        Task_Next();
        if( g_count >= 20){
            print_trace();
        }
    }
}

extern int r_main(){    
    uart_init();
    set_trace_test(12);

    services[0] = Service_Init();    
    
    Task_Create_RR(r,0);
    Task_Create_RR(r,0);
    Task_Create_RR(r2,0);
   
    Task_Terminate();
    return 0;
}

#endif