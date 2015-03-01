#ifdef USE_TEST_013

/*
System level tasks S1 and S2 subscribe to P1, periodic publisher (publishes 10).
System task S3 subscribes to P2, another periodic publisher (publishes 20).
System task S4 subscribes to R1, a RR publisher (publishes 30).

The periodic tasks are scheduled so that

P1:10      10      10      10      10
P2:  20      20      20      20      20

Since there are TWO subscribers to P1, [10,10] will be printed to the trace by the two subscribers.
Since there is ONE subscriber to P2, only [20] will be printed to the trace.
After P1 finishes and yields, RR runs before the scheduler dispatches P2 which preempts the RR task.
This cycle runs until each of the publishers printed out 10 times, after which 40 is printed out.

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
    set_trace_test(13);

    services[0] = Service_Init();
    services[1] = Service_Init();
    services[2] = Service_Init();
    services[3] = Service_Init();
    p_count = 2;
    
    /* Create system tasks which subscribe to services
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
