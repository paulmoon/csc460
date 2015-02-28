#ifdef USE_TEST_017

/*
    Desired Trace:    
    T017;10;0;20;0;10;1;20;1;10;2;20;2;10;0;20;0;10;1;20;1;10;2;20;2;...;30;
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

void setup() {

  //Clear timer config.
  TCCR3A = 0;
  TCCR3B = 0;  
  //Set to CTC (mode 4)
  TCCR3B |= (1<<WGM32);
  
  //Set prescaller to 256
  TCCR3B |= (1<<CS32);
  
  //Set TOP value (0.5 seconds)
  OCR3A = 31250;
  
  //Enable interupt A for timer 3.
  TIMSK3 |= (1<<OCIE3A);
  
  //Set timer to 0 (optional here).
  TCNT3 = 0;
}

int16_t volatile service_index = 0;
ISR(TIMER3_COMPA_vect)
{
  Profile1();
  Service_Publish(services[service_index],service_index);
  service_index  = (service_index + 1)%3;
  Profile2();
}

int counter = 0;
void s()
{
    int16_t arg = Task_GetArg();
    int16_t service_index =0;
    int16_t v;
    for(;;)
    {
        Service_Subscribe(services[service_index],&v);
        service_index = (service_index + 1)%3;
        add_to_trace(arg);
        add_to_trace(v);        
        if(counter >= 20)
        {
            break;
        }
        counter += 1;
    }

    Service_Publish(services[3],30);
    Task_Terminate();
}


extern int r_main(){    
    uart_init();

    services[0] = Service_Init();
    services[1] = Service_Init();
    services[2] = Service_Init();
    services[3] = Service_Init();

    setup();    

    /* create system tasks which subscribe to services
         Their argument determines the service they subscribe to */
    Task_Create_System(s,10); 
    Task_Create_System(s,20);
   
    int16_t v;
    Service_Subscribe(services[3],&v);
    add_to_trace(v);
    print_trace();
    
    Task_Terminate();
    return 0;
}

#endif
