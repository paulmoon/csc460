/*
 * cops_and_robbers.h
 *
 * Created: 2015-01-26 17:09:37
 *  Author: Daniel
 */

 // IR Transmitter
// -            = GND
// (middle) =
// s            = 2.2 Ohz -- 44

// IR Receiver
// -            = Gnd
// (middle) = Vcc
// s            = 3

// Radio
// CE   = 8
// CSN  = 9
// SCK  = 52
// MO   = 51
// MI   = 50
// IRQ  = 2
// VCC  = 47
// GND  = GND

// Roomba       ATMega
// Red(6.3v)        Vin
// Orange(GND)      GND
// Yellow(Tx)       19
// Green(Rx)        18
// DD               20


#ifndef COPS_AND_ROBBERS_H_
#define COPS_AND_ROBBERS_H_

#include "avr/io.h"

typedef enum _roomba_nums {COP1 = 0, COP2, ROBBER1, ROBBER2} COPS_AND_ROBBERS;
extern uint8_t ROOMBA_ADDRESSES[][5];

extern uint8_t ROOMBA_FREQUENCIES[];

typedef enum _ir_commands{
	SEND_BYTE,
	REQUEST_DATA,
} IR_COMMANDS;

typedef enum _roomba_statues{
	ROOMBA_ALIVE,
	ROOMBA_DEAD
}ROOMBA_STATUSES;


#endif /* COPS_AND_ROBBERS_H_ */