#ifdef USE_TEST_016

/*
    Desired Trace:
    T016;1;1;2;2;3;3;4;4;5;5;...;19;19;20;20;
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
int16_t count = 1;

void p() {
	int16_t service_i = Task_GetArg();	
    for (;;) {
        Service_Publish(services[service_i],count);
        count++;
        Task_Next();
    }
}

void r() {
	int16_t v;
	int16_t service_i = Task_GetArg();

    for (;;) {
		Service_Subscribe(services[service_i], &v);
		add_to_trace(v);

        if (count > 20) {
            Service_Publish(services[2], 40);
            Task_Terminate();
        }

        Task_Next();
    }
}

extern int r_main(){    
    uart_init();
    set_trace_test(16);

    services[0] = Service_Init();
    services[1] = Service_Init();
    services[2] = Service_Init();
    
    Task_Create_RR(r, 0);
    Task_Create_RR(r, 0);
    Task_Create_Periodic(p, 0, 20, 2, 5);
   
    int16_t v;
    Service_Subscribe(services[2], &v);
    add_to_trace(v);
    print_trace();
    
    Task_Terminate();
    return 0;
}

#endif
