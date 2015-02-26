#ifdef USE_TEST_010

/*
    Desired Trace:    
    T010;0;0;1;-1;2;-2;0;0;1;-1;2;-2;0;0;1;-1;2;-2;...
*/
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "os.h"
#include "uart/uart.h"
#include "trace/trace.h"
#include "../profiler.h"
#include "../error_code.h"

SERVICE* services[3];

void s2(){
    int16_t v = 0;
    int16_t i = 0;
    for(;;){
        Service_Publish(services[i],v);
        i = (i + 1)%3;        
        Task_Next();
    }
}

int count = 0;
void s(){
    int16_t v;
    int16_t i = 0;    
    int16_t arg = Task_GetArg();

    while( count < 30){
        Service_Subscribe(services[i],&v);
        add_to_trace(v*arg);

        i = (i + 1)%3;
        count += 1;        
    }

    print_trace();
}

extern int r_main(){    
    uart_init();

    services[0] = Service_Init();
    services[1] = Service_Init();
    services[2] = Service_Init();
    
    Task_Create_System(s,0);
    Task_Create_System(s,-1);
    Task_Create_System(s2,0);    
   
    Task_Terminate();
    return 0;
}

#endif