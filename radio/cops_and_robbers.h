/*
 * cops_and_robbers.h
 *
 * Created: 2015-01-26 17:09:37
 *  Author: Daniel
 */ 


#ifndef _COPS_AND_ROBBERS_H_
#define _COPS_AND_ROBBERS_H_

#include "avr/io.h"

typedef enum _cops_and_robbers_e {COP1 = 0, COP2, ROBBER1, ROBBER2} COPS_AND_ROBBERS;

extern uint8_t ROOMBA_ADDRESSES[][5];
extern uint8_t ROOMBA_FREQUENCIES[];


typedef enum _ir_commands_e {
	SEND_BYTE,
	REQUEST_DATA,
	AIM_SERVO
} IR_COMMANDS;

#endif 