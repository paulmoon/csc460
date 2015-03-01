/*
Testing RR tasks subscribing to a periodic publisher.
    Desired Trace:
    T016;1;1;2;2;3;3;4;4;5;5;...;19;19;20;20;
*/

#ifdef USE_TEST_016

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "os.h"
#include "uart/uart.h"
#include "trace/trace.h"
#include "../profiler.h"
#include "../error_code.h"

void p(){    
    for(;;){
        SERVICE* s = Service_Init();
        Task_Next();
		Service_Publish(s, 50);
    }
}

extern int r_main(){    
    uart_init();

    Task_Create_Periodic(p,10,5,2,0);
    Task_Terminate();
    return 0;
}

#endif
