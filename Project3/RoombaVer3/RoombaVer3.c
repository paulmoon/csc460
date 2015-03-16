/*
 * RoombaVer3.c
 *
 * Created: 2015-03-05 13:32:56
 *  Author: Daniel
 */

//#define F_CPU 16000000UL

#include <avr/io.h>
#include "os.h"
#include "roomba.h"
#include "roomba_sci.h"
#include "radio.h"
// #include "timer.h"
// #include "ir.h"
#include "cops_and_robbers.h"
#include "uart.h"
#include "profiler.h"

// EVENT* radio_receive_event;
// EVENT* ir_receive_event;
// OS_TIMER roomba_timeout_timer;
// volatile uint8_t is_roomba_timedout = 0;

// COPS_AND_ROBBERS roomba_num = COP1;
// volatile ROOMBA_STATUSES current_roomba_status = ROOMBA_ALIVE;
// volatile uint8_t last_ir_value = 0;


// //Used to stop the Roomba when it gets shot.
// void roomba_Drive( int16_t velocity, int16_t radius )
// {
// 	Roomba_Send_Byte(DRIVE);
// 	Roomba_Send_Byte(HIGH_BYTE(velocity));
// 	Roomba_Send_Byte(LOW_BYTE(velocity));
// 	Roomba_Send_Byte(HIGH_BYTE(radius));
// 	Roomba_Send_Byte(LOW_BYTE(radius));
// }

// void radio_rxhandler(uint8_t pipenumber) {
// 	Event_Async_Signal(radio_receive_event);
// }

// //Handle expected IR values, record unexpected values to pass on via radio.
// //	(Get Roomba state via state packets)
// void ir_rxhandler() {
// 	uint8_t ir_value = IR_getLast();
// 	Event_Async_Signal(radio_receive_event);
// 	if(last_ir_value == IR_SHOOT) {
// 		current_roomba_status = ROOMBA_DEAD;
// 	} else if(roomba_num == COP1 && last_ir_value == IR_WAKE_COP1) {
// 		current_roomba_status = ROOMBA_ALIVE;
// 	} else if(roomba_num == COP2 && last_ir_value == IR_WAKE_COP2) {
// 		current_roomba_status = ROOMBA_ALIVE;
// 	} else if(roomba_num == ROBBER1 && last_ir_value == IR_WAKE_ROBBER1) {
// 		current_roomba_status = ROOMBA_ALIVE;
// 	} else if(roomba_num == ROBBER2 && last_ir_value == IR_WAKE_ROBBER2) {
// 		current_roomba_status = ROOMBA_ALIVE;
// 	} else {
// 		last_ir_value = ir_value;
// 	}
// }

// void handleRoombaPacket(radiopacket_t *packet) {
// 	//Filter out unwanted commands.
// 	if (packet->payload.command.command == START ||
// 		packet->payload.command.command == BAUD ||
// 		packet->payload.command.command == SAFE ||
// 		packet->payload.command.command == FULL ||
// 		packet->payload.command.command == SENSORS)
// 	{
// 		return;
// 	}

// 	//If the Roomba is dead it should not start moving.
// 	if(packet->payload.command.command != DRIVE || current_roomba_status == ROOMBA_ALIVE) {
// 		//Pass the command to the Roomba.
// 		Roomba_Send_Byte(packet->payload.command.command);
// 		for (int i = 0; i < packet->payload.command.num_arg_bytes; i++)
// 		{
// 			Roomba_Send_Byte(packet->payload.command.arguments[i]);
// 		}
// 	}

// 	// Set the radio's destination address to be the remote station's address
// 	Radio_Set_Tx_Addr(packet->payload.command.sender_address);

// 	// Update the Roomba sensors into the packet structure that will be transmitted.
// 	//Roomba_UpdateSensorPacket(1, &packet.payload.sensors.sensors);
// 	//Roomba_UpdateSensorPacket(2, &packet.payload.sensors.sensors);
// 	//Roomba_UpdateSensorPacket(3, &packet.payload.sensors.sensors);

// 	// send the sensor packet back to the remote station.
// 	packet->type = SENSOR_DATA;
// 	packet->payload.sensors.roomba_number = roomba_num;
// 	Radio_Transmit(packet, RADIO_RETURN_ON_TX);
// }

// void handleIRPacket(radiopacket_t *packet) {

// 	//Transmit data
// 	if(packet->payload.ir_command.ir_command == SEND_BYTE) {
// 		cli();
// 		IR_transmit(packet->payload.ir_command.ir_data);
// 		sei();
// 	}

// 	if(packet->payload.ir_command.ir_command == AIM_SERVO) {
// 		//Aim the servo!
// 	}

// 	// Set the radio's destination address to be the remote station's address
// 	Radio_Set_Tx_Addr(packet->payload.command.sender_address);

// 	//Return last IR command recieved
// 	packet->type = IR_DATA;

// 	Radio_Transmit(packet, RADIO_RETURN_ON_TX);
// }
// void handleStatusPacket(radiopacket_t *packet) {
// 	//Figure out if we are reviving the Roomba or not.
// 	if(packet->payload.status_command.revive_roomba) {
// 		current_roomba_status = ROOMBA_ALIVE;
// 	}

// 	Radio_Set_Tx_Addr(packet->payload.command.sender_address);
// 	packet->type = ROOMBA_STATUS_UPDATE;
// 	packet->payload.status_info.roomba_number = roomba_num;
// 	packet->payload.status_info.roomba_status = current_roomba_status;

// 	Radio_Transmit(packet, RADIO_RETURN_ON_TX);
// }

// void rr_roomba_controler() {
// 	//Start the Roomba for the first time.
// 	Roomba_Init();

// 	for(;;) {
// 		Event_Wait(radio_receive_event);

// 		//Restart the timeout timer.
// 		timer_reset(&roomba_timeout_timer);
// 		timer_resume(&roomba_timeout_timer);

// 		if(is_roomba_timedout) {
// 			is_roomba_timedout = 0;
// 			Roomba_Init();
// 		}


// 		// Stop the Roomba if it is dead.
// 		if(current_roomba_status == ROOMBA_DEAD) {
// 			roomba_Drive(0,500);
// 		}

// 		//Handle the packets
// 		RADIO_RX_STATUS result;
// 		radiopacket_t packet;
// 		do {
// 			result = Radio_Receive(&packet);
// 			if(result == RADIO_RX_SUCCESS || result == RADIO_RX_MORE_PACKETS) {
// 				//Handle Roomba Commands
// 				if(packet.type == COMMAND) {
// 					handleRoombaPacket(&packet);
// 				}

// 				//Handle IR Commands
// 				if(packet.type == IR_COMMAND) {
// 					handleIRPacket(&packet);
// 				}

// 				if(packet.type == REQUEST_ROOMBA_STATUS_UPDATE) {
// 					handleStatusPacket(&packet);
// 				}
// 			}
// 		} while (result == RADIO_RX_MORE_PACKETS);
// 	}
// }


// //Check if the Roomba has been idle long enough to
// //	put it to sleep. This timer is reset every time a
// //	packet arrives.
// void per_roomba_timeout() {
// 	timer_reset(&roomba_timeout_timer);
// 	timer_resume(&roomba_timeout_timer);
// 	for(;;){
// 		if(timer_value(&roomba_timeout_timer) > 60000) {
// 			timer_pause(&roomba_timeout_timer);
// 			if(!is_roomba_timedout) {
// 				is_roomba_timedout = 1;
// 				Roomba_Finish();
// 			}
// 		}
// 	}
// }

// int r_main(void)
// {

	// //Turn off radio power.
	// DDRL |= (1 << PL2);
	// PORTL &= ~(1<<PL2);
	// _delay_ms(500);
	// PORTL |= (1<<PL2);
	// _delay_ms(500);

	// //Initialize radio.
	// cli();
	// Radio_Init();
	// IR_init();
	// Radio_Configure_Rx(RADIO_PIPE_0, ROOMBA_ADDRESSES[ROOMBA_NUM], ENABLE);
	// Radio_Configure(RADIO_2MBPS, RADIO_HIGHEST_POWER);
	// sei();

	// radio_receive_event = Event_Init();
	// ir_receive_event = Event_Init();
	// Task_Create_RR(rr_roomba_controler,0);
 // 	Task_Create_Period(per_roomba_timeout,0,10,9,0);
// }

void radio_rxhandler(uint8_t pipernumber){}



int r_main(void)
{
	EnableProfileSample1();
	DisableProfileSample1();
}