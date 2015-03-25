/*
 * RoombaVer3.c
 *
 * Created: 2015-03-05 13:32:56
 *  Author: Daniel
 */

// IR Transmitter
// - 			= GND
// (middle)	=
// s 			= 2.2 Ohz -- 44

// IR Receiver
// -			= Gnd
// (middle)	= Vcc
// s 			= 3

// Radio
// CE 	= 8
// CSN 	= 9
// SCK 	= 52
// MO  	= 51
// MI 	= 50
// IRQ	= 2
// VCC 	= 47
// GND 	= GND

// Roomba  		ATMega
// Red(6.3v)		Vin
// Orange(GND)		GND
// Yellow(Tx)		19
// Green(Rx)		18
// DD 				20
/*
//#define F_CPU 16000000UL
#include <avr/io.h>
#include "os.h"
#include "roomba.h"
#include "roomba_sci.h"
#include "radio.h"
#include "timer.h"

#include "ir.h"
#include "../cops_and_robbers.h"
#include "../trace_uart/trace_uart.h"
#include "../trace/trace.h"
#include "../uart/uart.h"
#include "../profiler.h"
#include "../analog/analog.h"


SERVICE* radio_receive_service;
SERVICE* ir_receive_service;
OS_TIMER roomba_timeout_timer;

volatile uint8_t is_roomba_timedout = 0;

COPS_AND_ROBBERS roomba_num = COP1;
volatile ROOMBA_STATUSES current_roomba_status = ROOMBA_ALIVE;
volatile uint8_t last_ir_value = 0;


void radio_rxhandler(uint8_t pipenumber) {
	Service_Publish(radio_receive_service,0);
}

//Handle expected IR values, record unexpected values to pass on via radio.
//	(Get Roomba state via state packets)
void ir_rxhandler() {
	uint8_t ir_value = IR_getLast();
	//Service_Publish(radio_receive_service,0);
	Service_Publish(ir_receive_service,0);
	if(last_ir_value == IR_SHOOT) {
		current_roomba_status = ROOMBA_DEAD;
	} else if(roomba_num == COP1 && last_ir_value == IR_WAKE_COP1) {
		current_roomba_status = ROOMBA_ALIVE;
	} else if(roomba_num == COP2 && last_ir_value == IR_WAKE_COP2) {
		current_roomba_status = ROOMBA_ALIVE;
	} else if(roomba_num == ROBBER1 && last_ir_value == IR_WAKE_ROBBER1) {
		current_roomba_status = ROOMBA_ALIVE;
	} else if(roomba_num == ROBBER2 && last_ir_value == IR_WAKE_ROBBER2) {
		current_roomba_status = ROOMBA_ALIVE;
	} else {
		last_ir_value = ir_value;
	}
}

void handleRoombaPacket(radiopacket_t *packet) {
	//Filter out unwanted commands.
	if (packet->payload.command.command == START ||
		packet->payload.command.command == BAUD ||
		packet->payload.command.command == SAFE ||
		packet->payload.command.command == FULL ||
		packet->payload.command.command == SENSORS)
	{
		return;
	}

	//If the Roomba is dead it should not start moving.
	// if(packet->payload.command.command != DRIVE || current_roomba_status == ROOMBA_ALIVE) {
	if(current_roomba_status == ROOMBA_ALIVE) {
		//Pass the command to the Roomba.
		Roomba_Send_Byte(packet->payload.command.command);
		for (int i = 0; i < packet->payload.command.num_arg_bytes; i++)
		{
			Roomba_Send_Byte(packet->payload.command.arguments[i]);
		}
	}

	// Set the radio's destination address to be the remote station's address
	Radio_Set_Tx_Addr(packet->payload.command.sender_address);

	// Update the Roomba sensors into the packet structure that will be transmitted.
	//Roomba_UpdateSensorPacket(EXTERNAL, &packet.payload.sensors.sensors);
	//Roomba_UpdateSensorPacket(CHASSIS, &packet.payload.sensors.sensors);
	//Roomba_UpdateSensorPacket(INTERNAL, &packet.payload.sensors.sensors);

	// send the sensor packet back to the remote station.
	packet->type = SENSOR_DATA;
	packet->payload.sensors.roomba_number = roomba_num;
	Radio_Transmit(packet, RADIO_RETURN_ON_TX);
}

void handleIRPacket(radiopacket_t *packet) {
	//Transmit data
	if(packet->payload.ir_command.ir_command == SEND_BYTE) {
		cli();
		IR_transmit(packet->payload.ir_command.ir_data);
		sei();
	}

	// Set the radio's destination address to be the remote station's address
	Radio_Set_Tx_Addr(packet->payload.command.sender_address);

	//Return last IR command received
	packet->type = IR_DATA;
	packet->payload.ir_data.roomba_number = roomba_num;
	packet->payload.ir_data.ir_data = IR_getLast();
	Radio_Transmit(packet, RADIO_RETURN_ON_TX);
}

void handleStatusPacket(radiopacket_t *packet) {
	//Figure out if we are reviving the Roomba or not.
	if(packet->payload.status_command.revive_roomba) {
		current_roomba_status = ROOMBA_ALIVE;
	}

	Radio_Set_Tx_Addr(packet->payload.command.sender_address);
	packet->type = ROOMBA_STATUS_UPDATE;
	packet->payload.status_info.roomba_number = roomba_num;
	packet->payload.status_info.roomba_status = current_roomba_status;

	Radio_Transmit(packet, RADIO_RETURN_ON_TX);
}

void rr_roomba_controler() {
	//Start the Roomba for the first time.
	Roomba_Init();
	int16_t value;

	for(;;) {
		Service_Subscribe(radio_receive_service,&value);

		// //Restart the timeout timer.
		timer_reset(&roomba_timeout_timer);
		timer_resume(&roomba_timeout_timer);
		if(is_roomba_timedout) {
			is_roomba_timedout = 0;
			Roomba_Init();
		}

		// Stop the Roomba if it is dead.
		if(current_roomba_status == ROOMBA_DEAD) {
			Roomba_Drive(0,500);
		}

		//Handle the packets
		RADIO_RX_STATUS result;
		radiopacket_t packet;
		do {
			result = Radio_Receive(&packet);

			if(result == RADIO_RX_SUCCESS || result == RADIO_RX_MORE_PACKETS) {
				//Profile5();

				if(packet.type == COMMAND) {
					handleRoombaPacket(&packet);
				}

				//Handle IR Commands
				if(packet.type == IR_COMMAND) {
					handleIRPacket(&packet);
				}

				if(packet.type == REQUEST_ROOMBA_STATUS_UPDATE) {
					handleStatusPacket(&packet);
				}
			}
		} while (result == RADIO_RX_MORE_PACKETS);
	}
}

//Check if the Roomba has been idle long enough to
//	put it to sleep. This timer is reset every time a
//	packet arrives.
void per_roomba_timeout() {
	timer_reset(&roomba_timeout_timer);
	timer_resume(&roomba_timeout_timer);
	for(;;){
		if(timer_value(&roomba_timeout_timer) > 60000) {
			timer_pause(&roomba_timeout_timer);
			if(!is_roomba_timedout) {
				is_roomba_timedout = 1;
				Roomba_Finish();
			}
		}

		Task_Next();
	}
}

void send_packet_task()
{
	// Set the radio's destination address to be the remote station's address
	Radio_Set_Tx_Addr(ROOMBA_ADDRESSES[COP1]);
	int16_t vel = 50;
	int16_t rad = 8000;

	for(;;){
		radiopacket_t packet;
		packet.type = COMMAND;
		for(int i =0;i < 5; ++i){
			packet.payload.command.sender_address[i] = ROOMBA_ADDRESSES[COP2][i];
		}
		packet.payload.command.command = DRIVE;
		packet.payload.command.num_arg_bytes = 4;
		vel = -vel;
		packet.payload.command.arguments[0] = HIGH_BYTE(vel);
		packet.payload.command.arguments[1] = LOW_BYTE(vel);
		packet.payload.command.arguments[2] = HIGH_BYTE(rad);
		packet.payload.command.arguments[3] = LOW_BYTE(rad);

		Radio_Transmit(&packet, RADIO_RETURN_ON_TX);
		// Radio_Transmit(&packet, RADIO_WAIT_FOR_TX);
		Task_Next();
	}
}

void dump_trace()
{
	print_trace();
}

void jordan1()
{
	for(;;){
		uint16_t a = Analog_read(9);
		add_to_trace(a);

		if(is_trace_full()){
			Task_Create_RR(dump_trace,0);
			Task_Terminate();
		}
		Task_Next();
	}
}

#define JORDAN_DEBUG_INIT (DDRB |= (1 << PB4));
#define JORDAN_DEBUG_ON (PORTB |= (1<<PB4));
#define JORDAN_DEBUG_OFF (PORTB &= ~(1<<PB4));

void _blink_led()
{
	int16_t count = 2*Task_GetArg();
	while(count >=0)
	{
		// TOGGLE the LED
		PORTB ^= ( 1 << PB4);
		--count;
		Task_Next();
	}
	JORDAN_DEBUG_OFF
}

void blink_led(int16_t num_blinks,int16_t delay)
{
	JORDAN_DEBUG_INIT
	Task_Create_Periodic(_blink_led,num_blinks,delay,1,Now() + 1);
}


int r_main(void)
{
	//Turn off radio power.
	DDRL |= (1 << PL2);
	PORTL &= ~(1<<PL2);
	_delay_ms(500);
	PORTL |= (1<<PL2);
	_delay_ms(500);

	trace_uart_init();
	Analog_init();

	//Initialize radio.
	cli();
	Radio_Init();
	IR_init();
	// Radio_Configure_Rx(RADIO_PIPE_0, ROOMBA_ADDRESSES[roomba_send], ENABLE);
	// Radio_Configure_Rx(RADIO_PIPE_0, ROOMBA_ADDRESSES[COP2], ENABLE);
	Radio_Configure_Rx(RADIO_PIPE_0, ROOMBA_ADDRESSES[COP1], ENABLE);
	//Radio_Configure_Rx(RADIO_PIPE_0, ROOMBA_ADDRESSES[roomba_send], ENABLE);

	Radio_Configure(RADIO_2MBPS, RADIO_HIGHEST_POWER);
	sei();

	radio_receive_service = Service_Init();
	ir_receive_service = Service_Init();

	blink_led(10,20);
	//Task_Create_Periodic(jordan1,0,20,10,252);

	// Task_Create_Periodic(send_packet_task,0,200,100,252);
	//Task_Create_RR(rr_roomba_controler,0);

	//Task_Create_Periodic(poll_pin, 0, 100, 10, 300);
	// Task_Create_Periodic(check_button,0,20,9,250);
	// Task_Create_Periodic(send_packet_task,0,100,10,252);
//	Task_Create_Periodic(p,0,200,9,251);
	//Task_Create_Periodic(transmit_ir,0,250,10, 303);
	//Task_Create_RR(transmit_ir, 0);
	Task_Terminate();
	return 0 ;
}
*/