/*
 * RoombaVer3.c
 *
 * Created: 2015-03-05 13:32:56
 *  Author: Daniel
 */

//#define F_CPU 16000000UL

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

#include <avr/io.h>
#include "os.h"
#include "roomba.h"
#include "roomba_sci.h"
#include "radio.h"
#include "timer.h"

#include "ir.h"
#include "cops_and_robbers.h"
#include "uart.h"
#include "profiler.h"

SERVICE* radio_receive_service;
SERVICE* ir_receive_service;
OS_TIMER roomba_timeout_timer;

volatile uint8_t is_roomba_timedout = 0;

COPS_AND_ROBBERS roomba_num = COP1;
volatile ROOMBA_STATUSES current_roomba_status = ROOMBA_ALIVE;
volatile uint8_t last_ir_value = 0;

#ifndef sbi
	#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

// void fireGun(radiopacket_t* packet,uint8_t b);
// void moveCommand(radiopacket_t* packet, int16_t spd, int16_t deg);


void radio_rxhandler(uint8_t pipenumber) {
	Service_Publish(radio_receive_service,0);
}

//Handle expected IR values, record unexpected values to pass on via radio.
//	(Get Roomba state via state packets)
void ir_rxhandler() {
	uint8_t ir_value = IR_getLast();
	Service_Publish(radio_receive_service,0);
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
	if(packet->payload.command.command != DRIVE || current_roomba_status == ROOMBA_ALIVE) {
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
	//Roomba_UpdateSensorPacket(1, &packet.payload.sensors.sensors);
	//Roomba_UpdateSensorPacket(2, &packet.payload.sensors.sensors);
	//Roomba_UpdateSensorPacket(3, &packet.payload.sensors.sensors);

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

	if(packet->payload.ir_command.ir_command == AIM_SERVO) {
		//Aim the servo!
	}

	// Set the radio's destination address to be the remote station's address
	Radio_Set_Tx_Addr(packet->payload.command.sender_address);

	//Return last IR command recieved
	packet->type = IR_DATA;

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

void p(){
	for(;;){
		Service_Publish(radio_receive_service,0);
		Task_Next();
	}
}

void transmit_ir() {
	for (;;) {
		IR_transmit((uint8_t) 'A');
		Service_Publish(radio_receive_service,0);
		Task_Next();
	}
}

void rr_roomba_controler() {
	//Start the Roomba for the first time.
	Roomba_Init();
	int16_t value;
	static int jordan_state = 0;

	for(;;) {
		Service_Subscribe(radio_receive_service,&value);

		//Restart the timeout timer.
		timer_reset(&roomba_timeout_timer);
		timer_resume(&roomba_timeout_timer);
		if(is_roomba_timedout) {
			is_roomba_timedout = 0;
			Roomba_Init();
		}

		if( jordan_state == 0){
			jordan_state = 1;
			Roomba_Drive(20,8000);
		}else{
			jordan_state = 0;
			Roomba_Drive(-20,8000);
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

// void fireGun(radiopacket_t* packet,uint8_t b){
//   packet->type = IR_COMMAND;
  
//   pf_ir_command_t* cmd = &(packet->payload.ir_command);
//   setSenderAddress(cmd->sender_address,my_addr,5);  
//   cmd->ir_command = SEND_BYTE;
//   cmd->ir_data = b;
//   cmd->servo_angle = 0;  
// }

// void moveCommand(radiopacket_t* packet, int16_t spd, int16_t deg){  
//   packet->type = COMMAND;
  
//   pf_command_t* cmd = &(packet->payload.command);
//   setSenderAddress(cmd->sender_address,my_addr,5);
  
//   cmd->command = 137; // DRIVE op code
//   cmd->num_arg_bytes = 4;
  
//   // set the  speed
//   cmd->arguments[0] = HIGH_BYTE(spd);
//   cmd->arguments[1] = LOW_BYTE(spd);
  
//   // set the amount of rotation
//   cmd->arguments[2] = HIGH_BYTE(deg);
//   cmd->arguments[3] = LOW_BYTE(deg);
// }

// // util function to set the sender addresss
// void setSenderAddress(uint8_t* dest, uint8_t *src,int len){
//   for(int i = 0; i < len ;++i){
//     dest[i] = src[i];
//   }
// }

// // send packet code
// void send_packet_task(){
//   if( txflag == DONTSEND){return;}
//   Serial.println("sending packet");

//   if(txflag == FIREGUN){
//     // send a fire gun packet
//     fireGun(&send_packet,'A');
//   }else if(txflag == MOVECOMMAND){
//     // send a move packet
//     // map the x and y values to the correct ranges 
//     uint16_t x_value = input.x_value;
//     uint16_t y_value = input.y_value;
//     if( x_value == 2 && y_value == 2){
//       // stop  
//       moveCommand(&send_packet,0,(int16_t)0x8000);
//     } else if( x_value == 2){
//       // move straight
//       y_value = -mapSpeedValue(y_value);
//       moveCommand(&send_packet,y_value,(int16_t)0x8000);
//     } else if( y_value == 2){
//       // turn in place
//       uint16_t deg = -1;
//       if( x_value < 2 ){
//         deg = 1;
//       }
//       moveCommand(&send_packet,200,deg);
//     }else{
//       x_value = -mapRotValue(x_value); 
//       y_value = -mapSpeedValue(y_value);      
//       moveCommand(&send_packet,y_value,x_value);
//     }
//   }

//   // transmit the send packet
//   Radio_Transmit(&send_packet, RADIO_WAIT_FOR_TX);  
// }

// // recv_packet code
// void recv_packet_task(){
//   if(rxflag == 0){return;}
  
//   // remember always to read the packet out of the radio, even
//   // if you don't use the data.
//   if (Radio_Receive(&recv_packet) != RADIO_RX_MORE_PACKETS){
//     // if there are no more packets on the radio, clear the receive flag;
//     // otherwise, we want to handle the next packet on the next loop iteration.
//     rxflag = 0;
//   }

//   txflag = DONTSEND;
//   if( recv_packet.type == SENSOR_DATA){
//     Serial.println("SENSOR_DATA");
//   }else if(recv_packet.type == IR_DATA){
//     Serial.println("IR_DATA:");
//     Serial.println(recv_packet.payload.ir_data.roomba_number);
//     Serial.println(recv_packet.payload.ir_data.ir_data);
//   }else{
//     Serial.println("Unknown recv packet"); 
//   }  
// }

int analogRead(uint8_t pin) {
	uint8_t low, high;

	#if defined(ADCSRB) && defined(MUX5)
	// the MUX5 bit of ADCSRB selects whether we're reading from channels
	// 0 to 7 (MUX5 low) or 8 to 15 (MUX5 high).
	ADCSRB = (ADCSRB & ~(1 << MUX5)) | (((pin >> 3) & 0x01) << MUX5);
#endif
  
	// set the analog reference (high two bits of ADMUX) and select the
	// channel (low 4 bits).  this also sets ADLAR (left-adjust result)
	// to 0 (the default).
#if defined(ADMUX)
	ADMUX = (analog_reference << 6) | (pin & 0x07);
#endif

	// without a delay, we seem to read from the wrong channel
	//delay(1);

#if defined(ADCSRA) && defined(ADCL)
	// start the conversion
	sbi(ADCSRA, ADSC);

	// ADSC is cleared when the conversion finishes
	while (bit_is_set(ADCSRA, ADSC));

	// we have to read ADCL first; doing so locks both ADCL
	// and ADCH until ADCH is read.  reading ADCL second would
	// cause the results of each conversion to be discarded,
	// as ADCL and ADCH would be locked when it completed.
	low  = ADCL;
	high = ADCH;
#else
	// we dont have an ADC, return 0
	low  = 0;
	high = 0;
#endif

	// combine the two bytes
	return (high << 8) | low;
}

void poll_pin() {
	for (;;) {
		uart_write(&analogRead(9), 8);
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

	//Initialize radio.
	cli();
	Radio_Init();
	IR_init();
	Radio_Configure_Rx(RADIO_PIPE_0, ROOMBA_ADDRESSES[roomba_num], ENABLE);
	Radio_Configure(RADIO_2MBPS, RADIO_HIGHEST_POWER);
	sei();

	// PF7 = Vx, PF8 = Vy, PF9 = SW
	PORTF |= (1 << PF7);
	PORTF |= (1 << PF8);
	PORTF |= (1 << PF9);

	DDRF &= ~(1 << PF7);
	DDRF &= ~(1 << PF8);
	DDRF &= ~(1 << PF9);

	radio_receive_service = Service_Init();
	ir_receive_service = Service_Init();
	Task_Create_RR(rr_roomba_controler,0);
	Task_Create_Periodic(poll_pin, 0, 100, 10, 300);
	// Task_Create_Periodic(check_button,0,20,9,250);
	// Task_Create_Periodic(send_packet_task,0,100,10,252);
//	Task_Create_Periodic(p,0,200,9,251);
	//Task_Create_Periodic(transmit_ir,0,250,10, 303);
	//Task_Create_RR(transmit_ir, 0);
	Task_Terminate();
	return 0 ;
}
