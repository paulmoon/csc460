#ifdef USE_TEST_008

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "os.h"
#include "uart/uart.h"
#include "trace/trace.h"
#include "../profiler.h"
#include "../error_code.h"


void r(){
    Task_Create_RR(r,0);
    for(;;){
        Task_Next();
    }
}

void s(){
    for(;;){
        Task_Create(s,0);
        Task_Next();
    }    
}

void p(){
    uint16_t p_count = Task_GetArg();
    for(;;){
        Task_Create(p,0,10,5,p_count++);
        Task_Next();
    }
}

extern int r_main(){    
    uart_init();

    Task_Create_RR(r,0);
    //Task_Create_System(s,0);
    //Task_Create_Periodic(p,0,10,5,0);
   
    Task_Terminate();
    return 0;
}

#endif