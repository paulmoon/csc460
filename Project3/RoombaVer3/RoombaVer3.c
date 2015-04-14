#if 0
#define USE_BASE_STATION
#include "BaseStation.cpp"

#else

#include <avr/io.h>
#include "os.h"
#include "roomba.h"
#include "roomba_sci.h"
#include "roomba_led_sci.h"
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

GAME_PLAYERS roomba_num = PLAYER1;
volatile ROOMBA_STATUSES current_roomba_status = ROOMBA_ALIVE;
volatile uint8_t last_ir_value = 0;

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
	//Profile2();
	JORDAN_DEBUG_INIT
	Task_Create_Periodic(_blink_led,num_blinks,delay,1,Now() + 1);
}


void radio_rxhandler(uint8_t pipenumber) {
	Profile1();
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
			//Profile1();
			if(result == RADIO_RX_SUCCESS || result == RADIO_RX_MORE_PACKETS) {

				// blink the LED
				for(int i = 0;i < 1000;++i){
					JORDAN_DEBUG_ON;
				}

				if(packet.type == COMMAND) {
					handleRoombaPacket(&packet);
				}

				if(packet.type == IR_COMMAND) {
					handleIRPacket(&packet);
				}

				if(packet.type == REQUEST_ROOMBA_STATUS_UPDATE) {
					handleStatusPacket(&packet);
				}
			}
		} while (result == RADIO_RX_MORE_PACKETS);

		JORDAN_DEBUG_OFF
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


int8_t led_bits = 0;
int8_t color = 0;
int8_t intensity = 0;
void shield_status(int i){
	if( i == 0) {
		// shield off
		led_bits &= ~(1 << 0);
	}else{
		// shield on
		led_bits |= (1 << 0);
	}
	Roomba_LED(led_bits,color,intensity);
}

void set_color(int type){
	if( type == 0){
		color = 255;
		intensity = 255;
	}else if( type == 1){
		color = 0;
		intensity = 255;
	}else{
		color = 0;
		intensity = 0;
	}
	Roomba_LED(led_bits,color,intensity);
}

void blink_roomba_led()
{
	for(;;){
		Roomba_led_debris(1);
		Task_Next();
		Roomba_led_debris(0);
		Task_Next();
		Roomba_led_spot(1);
		Task_Next();
		Roomba_led_spot(0);
		Task_Next();
		Roomba_led_dock(1);
		Task_Next();
		Roomba_led_dock(0);
		Task_Next();
		Roomba_led_warn(1);
		Task_Next();
		Roomba_led_warn(0);
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

	JORDAN_DEBUG_INIT
	Task_Create_RR(rr_roomba_controler,0);
	//Task_Create_Periodic(blink_roomba_led,0,200,10,252);

	Task_Terminate();
	return 0 ;
}

#endif