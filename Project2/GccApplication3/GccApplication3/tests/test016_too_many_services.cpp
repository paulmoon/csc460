/*
Testing RR tasks subscribing to a periodic publisher.
    Desired Trace:
    T016;1;1;2;2;3;3;4;4;5;5;...;19;19;20;20;
*/

#ifdef USE_TEST_016

/*
	Test to see that the OS aborts if you try to create
	too many services.
*/
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "os.h"
#include "uart/uart.h"
#include "trace/trace.h"
#include "../profiler.h"
#include "../error_code.h"

void s(){    
    for(;;){
        SERVICE* s = Service_Init();
        Task_Next();
		Service_Publish(s, 50);
    }
}

extern int r_main(){    
    uart_init();
		
    Task_Create_System(s,0);
    Task_Terminate();
    return 0;
}

#endif
