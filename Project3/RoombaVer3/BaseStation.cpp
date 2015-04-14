#ifdef USE_BASE_STATION

//#define F_CPU 16000000UL
#include <avr/io.h>
#include "os.h"
#include "roomba.h"
#include "roomba_sci.h"
#include "radio.h"
#include "timer.h"

#include "ir.h"
#include "cops_and_robbers.h"
#include "trace_uart/trace_uart.h"
#include "trace/trace.h"
#include "uart/uart.h"
#include "profiler.h"
#include "analog/analog.h"

SERVICE* radio_receive_service;
SERVICE* ir_receive_service;
OS_TIMER roomba_timeout_timer;

volatile uint8_t is_roomba_timedout = 0;

GAME_PLAYERS roomba_num = PLAYER2;
volatile ROOMBA_STATUSES current_roomba_status = ROOMBA_ALIVE;
volatile uint8_t last_ir_value = 0;

void radio_rxhandler(uint8_t pipenumber) {
    Profile1();
    Service_Publish(radio_receive_service,0);
}

//Handle expected IR values, record unexpected values to pass on via radio.
//  (Get Roomba state via state packets)
// NEED TO CHANGE!
void ir_rxhandler() {
    uint8_t ir_value = IR_getLast();
    //Service_Publish(radio_receive_service,0);
    Service_Publish(ir_receive_service,0);
    if(last_ir_value == IR_SHOOT) {
        current_roomba_status = ROOMBA_DEAD;
    } else if(roomba_num == PLAYER1 && last_ir_value == IR_WAKE_COP1) {
        current_roomba_status = ROOMBA_ALIVE;
    } else if(roomba_num == PLAYER2 && last_ir_value == IR_WAKE_COP2) {
        current_roomba_status = ROOMBA_ALIVE;
    } else if(roomba_num == PLAYER3 && last_ir_value == IR_WAKE_ROBBER1) {
        current_roomba_status = ROOMBA_ALIVE;
    } else if(roomba_num == PLAYER4 && last_ir_value == IR_WAKE_ROBBER2) {
        current_roomba_status = ROOMBA_ALIVE;
    } else {
        last_ir_value = ir_value;
    }
}

void send_packet_task()
{
    // Set the radio's destination address to be the remote station's address
    Radio_Set_Tx_Addr(ROOMBA_ADDRESSES[PLAYER1]);
    char code[30];

    for(;;){
        //Profile1();

        uint16_t vx = Analog_read(1);
        uint16_t vy = Analog_read(2);

        radiopacket_t packet;
        packet.type = COMMAND;
        for(int i =0;i < 5; ++i){
            packet.payload.command.sender_address[i] = ROOMBA_ADDRESSES[roomba_num][i];
        }
        packet.payload.command.command = DRIVE;
        packet.payload.command.num_arg_bytes = 4;

        // vx = ((vx /(1024/5)) - 2)*50;
        // vy = ((vy /(1024/5)) - 2)*100;

        // output the trace
        //EnableProfileSample1();
        //sprintf(code,"%d %d\n",vx,vy);
        //trace_uart_putstr(code);
        //DisableProfileSample1();
        //trace_uart_putchar((int8_t)vx);
        //trace_uart_putchar(' ');
        //trace_uart_putchar((int8_t)vy);
        //trace_uart_putchar('\n');

        packet.payload.command.arguments[0] = HIGH_BYTE(vx);
        packet.payload.command.arguments[1] = LOW_BYTE(vx);
        packet.payload.command.arguments[2] = HIGH_BYTE(vy);
        packet.payload.command.arguments[3] = LOW_BYTE(vy);

        // Radio_Transmit(&packet, RADIO_RETURN_ON_TX);
        Radio_Transmit(&packet, RADIO_WAIT_FOR_TX);
		Profile1();
        Task_Next();
    }
}

int r_main(void)
{
    //Turn off radio power.
    DDRL |= (1 << PL2);
    PORTL &= ~(1<<PL2);
    _delay_ms(500);
    PORTL |= (1<<PL2);
    _delay_ms(500);

    // initialize the trace + Analog modules
    trace_uart_init();
    Analog_init();

    //Initialize radio.
    cli();
    Radio_Init();
    IR_init();
    Radio_Configure_Rx(RADIO_PIPE_0, ROOMBA_ADDRESSES[roomba_num], ENABLE);
    // Radio_Configure(RADIO_2MBPS, RADIO_HIGHEST_POWER);
    Radio_Configure(RADIO_1MBPS, RADIO_HIGHEST_POWER);
    sei();

    radio_receive_service = Service_Init();
    ir_receive_service = Service_Init();

    Task_Create_Periodic(send_packet_task,0,20,10,252);
    Task_Terminate();
    return 0 ;
}

#endif