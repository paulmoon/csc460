#ifdef USE_TEST_013

/*
    Desired Trace:    
    T013;10;10;30;20;30;30;30;30;10;10;30;20;30;30;30;30;10;10;...;40
*/
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "os.h"
#include "uart/uart.h"
#include "trace/trace.h"
#include "../profiler.h"
#include "../error_code.h"

SERVICE* services[4];

void s(){
    int16_t v;    
    int16_t i = Task_GetArg();
    for(;;){
        Service_Subscribe(services[i],&v);
        add_to_trace(v);        
    }    
}

int p_count = 2;
void p1(){    
    int16_t v = Task_GetArg();
    int i;
    for(i = 0; i < 10 ; ++i){
        Service_Publish(services[0],v);
        Task_Next();
    }
    p_count -= 1;
}

void p2(){
    int16_t v = Task_GetArg();
    int i;
    for(i = 0;i < 10; ++i){
        Service_Publish(services[1],v);
        Task_Next();
    }
    p_count -= 1;
}

void r(){
    int16_t v = Task_GetArg();
    for(;;){
        Service_Publish(services[2],v);

        // delay one tick
        _delay_ms(5);

        // publish to signal that we want to print the trace.
        if( p_count <= 0){
            Service_Publish(services[3],40);
            Task_Terminate();            
        }
        Task_Next();
    }
}


extern int r_main(){    
    uart_init();

    services[0] = Service_Init();
    services[1] = Service_Init();
    services[2] = Service_Init();
    services[3] = Service_Init();
    p_count = 2;
    

    /* create system tasks which subscribe to services
         Their argument determines the service they subscribe to */
    Task_Create_System(s,0); 
    Task_Create_System(s,0);
    Task_Create_System(s,1);
    Task_Create_System(s,2);

    /* Create producers for the services */
    Task_Create_Periodic(p1,10,5,2,0);
    Task_Create_Periodic(p2,20,5,2,1);
    Task_Create_RR(r,30); 
   
    int16_t v;
    Service_Subscribe(services[3],&v);
    add_to_trace(v);
    print_trace();
    
    Task_Terminate();
    return 0;
}

#endif
