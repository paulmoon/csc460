#ifdef USE_TEST_011

/*
Testing periodic task publishing to a service that a system level task subscribes to.
A system task will subscribe to a periodic task which publishes values from 0 to 20.
    Desired Trace:
    T011;0;1;2;3;4;5;6;...;18;19;20;
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "os.h"
#include "uart/uart.h"
#include "trace/trace.h"
#include "../profiler.h"
#include "../error_code.h"

SERVICE* services[1];

void dump_trace()
{
    print_trace();
}

void p() {
    int16_t count = 0;

    while (count <= 20) {
        Service_Publish(services[0], count++);
        Task_Next();
    }

	Task_Create_RR(dump_trace,0);
}

void s() {
    int16_t v;
    for (;;) {
        Service_Subscribe(services[0], &v);
        add_to_trace(v);
    }
}

extern int r_main(){    
    uart_init();
    set_trace_test(11);

    services[0] = Service_Init();    
    
    Task_Create_Periodic(p, 10, 10, 2, 5);
    Task_Create_System(s, 0);
    Task_Terminate();
    return 0;
}

#endif