#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "kernel.h"
#include "os.h"
#include "uart/uart.h"
#include "trace/trace.h"
#include "profiler.h"

#define USE_TEST_018
#include "tests/test000_now.cpp"
#include "tests/test001_create_system_task.cpp"
#include "tests/test002_create_rr_task.cpp"
#include "tests/test003_create_periodic_task.cpp"
#include "tests/test004_multiple_periodic.cpp"
#include "tests/test005_3_plus_periodic.cpp"
#include "tests/test006_all_task_types.cpp"
#include "tests/test007_periodic_errors.cpp"
#include "tests/test008_create_too_many_tasks.cpp"
#include "tests/test009_service_init.cpp"
#include "tests/test010_service_system.cpp"
#include "tests/test011_service_periodic.cpp"
#include "tests/test012_service_rr.cpp"
#include "tests/test013_service_all.cpp"
#include "tests/test014_service_RR_subscribe.cpp"
#include "tests/test015_periodic_cant_subscribe.cpp"
#include "tests/test016_too_many_services.cpp"
#include "tests/test017_service_interrupt.cpp"
#include "tests/test018_latest_publish.cpp"


#ifdef USE_MAIN
extern int r_main(){
	DDRB |= _BV(PB7);
	PORTB |= _BV(PB7); // turn on
	PORTB &= ~(_BV(PB7)); // turn off
	
	return 0;
}
#endif
