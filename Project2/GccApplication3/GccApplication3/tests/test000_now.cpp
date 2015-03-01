#ifdef USE_TEST_000

/*
    Desired Trace
    T000;0;25;50;76;100;125;...;6420;

    Note that every 4th entry, it will drift by 1ms
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "os.h"
#include "uart/uart.h"
#include "trace/trace.h"

extern int r_main(){    
    uart_init();
    set_trace_test(0);
    
	int j;
	uint16_t v;		
    for(j = 0; j < TRACE_ARRAY_SIZE; ++j){
        v = Now();
        add_to_trace(v);
        _delay_ms(25);
    
    }	
    print_trace();	
    return 0;
}

#endif
