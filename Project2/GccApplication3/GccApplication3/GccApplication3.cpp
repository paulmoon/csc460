#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "kernel.h"
#include "os.h"
#include "uart/uart.h"
#include "trace/trace.h"
#include "profiler.h"

#define USE_MAIN
#include "tests/test000_now.cpp"
#include "tests/test001_create_system_task.cpp"
#include "tests/test002_create_rr_task.cpp"
#include "tests/test003_create_periodic_task.cpp"
#include "tests/test004_multiple_periodic.cpp"
#include "tests/tefst005_3_plus_periodic.cpp"
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
#include "tests/test019_invalid_service_descriptor.cpp"


#ifdef USE_MAIN

void s()
{
    Task_Terminate();
}

void p()
{
    for(;;)    {
        Task_Next();
    }
}


SERVICE* services[1];
void r2(){
	int16_t v;
	for(;;){
		Service_Subscribe(services[0],&v);		
	}
}

void p2(){		
	for(;;){
		Service_Publish(services[0],0);
		Task_Next();		
	}
}

void p3(){
	Task_Create_System(s,0);
	Task_Terminate();
}

void r3()
{
	Task_Create_Periodic(p,0,10,1,0);	
}

SERVICE* serv1;
void p4(){
	for(;;){
		Profile1();
		Service_Publish(serv1,0);
		Task_Next();
	}
}

extern int r_main(){
    uart_init();
	serv1 = Service_Init();

    /*int i = 0;
    for(i =0 ;i < 7; ++i )
    {        
        //Task_Create_System(s,0);

        // EnableProfileSample1();
        // Task_Create_RR(s,0);
        // DisableProfileSample1();

        // EnableProfileSample1();
        //Task_Create_Periodic(s,0,10,5,i);
        // DisableProfileSample1();
    }*/
	
	/*services[0] = Service_Init();
	
	Task_Create_Periodic(p2,0,10,1,10);
	int i= 0;
	for(i= 0;i < 5; ++i){
		Task_Create_RR(r2,0);
	}
	*/
	/*for(;;){
		Now();
		_delay_ms(8);		
	}
	*/
	
	//Task_Create_Periodic(p3,0,10,1,0);	
	//Task_Create_RR(r3,0);
	
	Task_Create_Periodic(p4,0,200,9,251);
	Task_Terminate();
	return 0;
}
#endif
