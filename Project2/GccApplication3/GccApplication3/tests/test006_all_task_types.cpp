#ifdef USE_TEST_006

/*
Test all tasks types running together.

Create a system task A
Create two periodic tasks B and C
Create two round-robin tasks D and E
Task A will run first and create a new round robin task F, and then terminate.
Task B is periodic and runs immediately.
The spare time after Task B yields is used by the first RR task (D)
Task D will run and then be pre-empted by task C.
Task C will run and then yield the processor.
The remaining RR tasks D,E,F will now run until the next prempt from a periodic task.

A 1 1
B     2          2          2
C         3          3           3
D       4          4          4            
E           5          5           5
F             6          6           6 ...

Desired trace:
T006;0;0;1;1;2;4;3;5;6;2;3;2;3;2;4;3;5;6;2;3;2;3;2;4;3;5;6;2;3;2;3;2;4;3;5;6;2;3;2;3;2;4;3;5...
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "os.h"
#include "uart/uart.h"
#include "trace/trace.h"


void dump_trace(){
    Task_Next();
    print_trace();
}

int p_counter = 0;
void p_task(){
    uint16_t v = Task_GetArg();

    p_counter += 1;
    for(;;){
        add_to_trace(v);
        if( is_trace_full() ){
            break;
        }
        Task_Next();
    }
    p_counter -= 1;

    if( p_counter <= 0){                
        Task_Create_RR(dump_trace,0);
    }    
    Task_Terminate();
}

void r(){
	uint16_t v = Task_GetArg();
	int i = 10;
	while( i > 0){
		add_to_trace(v);
		--i;
		_delay_ms(25);
		Task_Next();
	}
}

void s1(){
    uint16_t v = Task_GetArg();
    add_to_trace(v);
    Task_Create_RR(r,6);
    add_to_trace(v);
    Task_Terminate();
}

extern int r_main(){    
    uart_init();
    set_trace_test(6);

    add_to_trace(0);
    
    Task_Create_System(s1,1);
    Task_Create_Periodic(p_task,2,5,2,0);
    Task_Create_Periodic(p_task,3,5,2,1);
    Task_Create_RR(r,4);
    Task_Create_RR(r,5);

    add_to_trace(0);

    Task_Terminate();
    return 0;
}

#endif