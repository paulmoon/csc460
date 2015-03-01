#ifdef USE_TEST_015

/*
Periodic tasks were defined so that they cannot subscribe to a publisher.
This test simply has a RR publisher and a periodic subscriber.
Upon running this test, the LED should blink 12 times for the error ERR_RUN_12_PERIODIC_CANT_SUBSCRIBE.
    Desired Trace: None
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

void p() {
    for(;;) {
        int16_t v;
        Service_Subscribe(services[0], &v);
        add_to_trace(v);

        // This should never happen since periodic tasks cannot subscribe.
        if (v > 1) {
            print_trace();
            Task_Terminate();
        }

        Task_Next();
    }
}

void r() {
    int16_t count = 0;

    for (;;) {
        Service_Publish(services[0], count);
        count++;
        Task_Next();
    }
}

extern int r_main(){    
    uart_init();
    set_trace_test(15);

    services[0] = Service_Init();

    Task_Create_RR(r, 0);
    // Periodic tasks cannot subscribe. Expected: OS abort with code ERR_RUN_12_PERIODIC_CANT_SUBSCRIBE
    Task_Create_Periodic(p, 10, 10, 2, 5);

    Task_Terminate();
    return 0;
}

#endif
