#ifdef USE_TEST_000

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "os.h"
#include "uart/uart.h"
#include "trace/trace.h"

extern int r_main(){    
    uart_init();
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