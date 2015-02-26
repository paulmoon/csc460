#ifdef USE_TEST_011

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "os.h"
#include "uart/uart.h"
#include "trace/trace.h"
#include "../profiler.h"
#include "../error_code.h"

SERVICE* services[3];

void p(){
    int16_t v;
    for(;;){
        Service_Subscribe(services[0],&v);
    }
}

extern int r_main(){    
    uart_init();
    
    services[0] = Service_Init();    
    Task_Create_Periodic(p,0,10,2,0); 


    Task_Terminate();
    return 0;
}

#endif