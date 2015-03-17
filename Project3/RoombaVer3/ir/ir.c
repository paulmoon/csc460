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
void IR_init() {

	DDRL |= (1 << PL3)  | ( 1 << PL5);
	TCCR5A = 0;
	TCCR5B = 0;

	TIMSK5 &= ~(1 << OCIE5C);

	// fast pwm
	TCCR5A |= (1<<WGM50) | (1<<WGM51);
	TCCR5B |= (1<<WGM52) | (1<<WGM53);

	// output to pin 44, for output C
	TCCR5A |= (1 << COM5C1);

	// no prescaler
	TCCR5B |= (1 << CS50);

	OCR5A = 421; // 38 Khz
	OCR5C = 210; // 50 % duty


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
	OCR2A = 421; // 38khz
	OCR2B = 210; // 50% duty cycle

	// Interrupt Timer 3.
	// clear the control registers
	TCCR3A = 0;
	TCCR3B = 0;
	//Leave on normal mode.
	//No prescaller
	TCCR3B |= (1<<CS10);
	//Make sure interrupt is disabled until external interrupt
	TIMSK3 &= ~(1<<OCIE3A);

	//Setup the input interrupt on pin 3 (PE5/INT4)
	DDRE &= ~(1<<PE5);
	EICRB |= (1<<ISC51) | (1<<ISC50);

// 	EIMSK |= (1<<INT6);
// 	PCMSK2 |= (1<<)
// 	EICRB |=
}



//Receiving a signal.
// ISR(INT5_vect) {
// 	if(!is_receiving) {
// 		//Start a new byte, start the timers.
// 		is_receiving = 1;
// 		currentBit = 0;
// 		currentByte = 0;

// 		//Clear any existing timer interrupts.
// 		TIFR3 |= (1<<OCF3A);

// 		//Delay by 1.5 bit lengths.
// 		OCR3A = TCNT3 + 12000;
// 		TIMSK3 |= (1<<OCIE3A);
// 	}
// }

// //Read a new arriving signal.
// ISR(TIMER3_COMPA_vect) {
// 	if(is_receiving) {
// 		if(PINE & (1<<PE4)) {
// 			currentByte |= (1<<currentBit);
// 		}

// 		++currentBit;
// 		OCR3A += 8000;

// 		if(currentBit >= 8) {
// 			is_receiving = 0;
// 			TIMSK3 &= ~(1<<OCIE3A);

// 			TIFR3 |= (1<<OCF3A);
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

	TIMSK3 &= ~(1<<OCIE3A);
	TIFR3 |= (1<<OCF3A);

	is_receiving = 0;
}

void mark() {
	// TCCR2A |= (1<<COM2A1);;
	TCCR5A |= (1<<COM5C1);
//	PORTC |= (1 << PC2);
	_delay_us(500);
}
void space() {
	// TCCR2A &= ~(1 << COM2A1);
	TCCR5A &= ~(1 << COM5C1);
//	PORTC &= ~(1 << PC2);
	_delay_us(500);
}

void IR_transmit(uint8_t data) {
	uint8_t sreg = SREG;
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

	SREG = sreg;
	//sei();
}