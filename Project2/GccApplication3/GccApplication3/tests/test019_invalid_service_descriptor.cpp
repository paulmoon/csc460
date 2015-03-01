    /*
Test that publishing or subscribing to an invalid service throws an
ERR_RUN_11_INVALID_SERVICE_DESCRIPTOR error.

    Desired Trace:
    T019;
*/

#ifdef USE_TEST_019

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "os.h"
#include "uart/uart.h"
#include "trace/trace.h"
#include "../profiler.h"
#include "../error_code.h"

SERVICE* services[2];

void p() {
    int16_t v;
    for(;;) {
        // Using an invalid Service descriptor
        Service_Publish(services[1], 50);
        // This should fail as well, if the publish code is commented out
        // Service_Subscribe(services[1], &v);
        Task_Next();
    }
}

extern int r_main() {
    uart_init();
    set_trace_test(19);

    // Only the first Service is initialized in the services array
    services[0] = Service_Init();
    Task_Create_Periodic(p,10,5,2,0);
    Task_Terminate();
    return 0;
}

#endif
