/*
 * ir.c
 *
 * Created: 2015-01-28 12:15:57
 *  Author: Daniel
 */

#include "ir.h"
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

extern void ir_rxhandler();

volatile uint8_t is_receiving = 0;
volatile uint8_t is_transmitting = 0;
volatile uint8_t currentBit = 0;
volatile uint8_t currentByte = 0;
volatile uint8_t outputByte = 0;


// enable the interrupt handler for the timer register
// set the control and status register
	// mode, prescalar
// output compare register ( when to fire the thingy)

//Timer 2 runs PWM.
//Timer 1 runs signaling.
//Output on pin 10 (PB4/OC2A)
//Input on pin 3 (PE5/INT5)
/*
void IR_init() {

	//PWM Timer 2
	DDRB |= (1<<PB4);

	// clear the control registers
	TCCR2A = 0;
	TCCR2B = 0;

	//Set to Fast PWM mode 15
	TCCR2A |= (1<<WGM20) | (1<<WGM21);
	TCCR2B |= (1<<WGM22);
	//No Prescaller
	TCCR2B |= (1<<CS20);

	OCR1A = 421; // 38khz
	OCR1C = 210; // 50% duty cycle

	//Interrupt Timer 1.
	// clear the control registers
	TCCR1A = 0;
	TCCR1B = 0;
	//Leave on normal mode.
	//No prescaller
	TCCR1B |= (1<<CS10);
	//Make sure interrupt is disabled until external interrupt
	TIMSK1 &= ~(1<<OCIE1A);

	//Setup the input interrupt on pin 3 (PE5/INT4)
	DDRE &= ~(1<<PE5);
	EICRB |= (1<<ISC51) | (1<<ISC50);

// 	EIMSK |= (1<<INT6);
// 	PCMSK2 |= (1<<)
// 	EICRB |=
}
*/


//Receiving a signal.
ISR(INT5_vect) {
	if(!is_receiving) {
		//Start a new byte, start the timers.
		is_receiving = 1;
		currentBit = 0;
		currentByte = 0;
		//Clear any existing timer interrupts.
		TIFR1 |= (1<<OCF1A);
		//Delay by 1.5 bit lengths.
		OCR1A = TCNT1 + 12000;
		TIMSK1 |= (1<<OCIE1A);
	}
}

//Read a new arriving signal.
// ISR(TIMER1_COMPA_vect) {
// 	if(is_receiving) {
// 		if(PINE & (1<<PE4)) {
// 			currentByte |= (1<<currentBit);
// 		}

// 		++currentBit;
// 		OCR1A += 8000;

// 		if(currentBit >= 8) {
// 			is_receiving = 0;
// 			TIMSK1 &= ~(1<<OCIE1A);
// 			TIFR1 |= (1<<OCF1A);
// 			EIFR |= (1<<INTF4);
// 		}
// 	}else if (is_transmitting) {

// 	}

// }

void enable_interupt() {
	//Clears existing interrupts.
	EIFR |= (1<<INTF4);
	EIMSK |= (1<<INT4);
}

void disable_interupt() {
	EIMSK &= ~(1<<INT4);

	TIMSK1 &= ~(1<<OCIE1A);
	TIFR1 |= (1<<OCF1A);

	is_receiving = 0;
}

void mark() {
	TCCR2A |= (1<<COM2A1);;
//	PORTC |= (1 << PC2);
	_delay_us(500);
}
void space() {
	TCCR2A &= ~(1 << COM2A1);
//	PORTC &= ~(1 << PC2);
	_delay_us(500);
}

void IR_transmit(uint8_t data) {
	cli();
	disable_interupt();
	mark();
	space();
	for(int i = 0; i < 8; i++) {
		if(((data >> i) & 0x1)) {
			mark();
		} else {
			space();
		}
	}
	space();
	enable_interupt();
	sei();
}