#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "kernel.h"
#include "os.h"
#include "uart/uart.h"
#include "trace/trace.h"
#include "profiler.h"

#define USE_TEST_003
#include "tests/test000_now.cpp"
#include "tests/test001_create_system_task.cpp"
#include "tests/test002_rr_task.cpp"
#include "tests/test003_periodic_tasks.cpp"
#include "tests/test004_multiple_periodic.cpp"

#ifdef USE_MAIN
extern int r_main(){
	DDRB |= _BV(PB7);
	PORTB |= _BV(PB7); // turn on
	PORTB &= ~(_BV(PB7)); // turn off
	
	return 0;
}
#endif
