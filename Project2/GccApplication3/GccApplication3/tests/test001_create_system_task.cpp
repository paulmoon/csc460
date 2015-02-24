#ifdef USE_TEST_001
/*
Desired trace:
T001;0;0;1;2;1;2;2;3;
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "os.h"
#include "uart/uart.h"
#include "trace/trace.h"


void system1(){
    add_to_trace(1);
    Task_Next();
    add_to_trace(1);
	/* end by calling terminate */
    Task_Terminate();
}

void system3(){
    add_to_trace(3);
    print_trace();
	/* non-implicit return */
}

void system2(){
    add_to_trace(2);
    Task_Next();
    add_to_trace(2);
    Task_Create_System(system3,3);
    add_to_trace(2);
	/* explicit return */
    return;
}

extern int r_main(){    
    uart_init();
    set_trace_test(1);

    add_to_trace(0);
    Task_Create_System(system1,1);
    Task_Create_System(system2,2);    
    add_to_trace(0);

    Task_Terminate();
	return 0;
}

#endif