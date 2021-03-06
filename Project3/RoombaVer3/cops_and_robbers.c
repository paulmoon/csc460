/*
 * cops_and_robbers.h
 *
 * Created: 2015-01-26 17:09:37
 *  Author: Daniel
 */

#include "cops_and_robbers.h"
#include "avr/io.h"

uint8_t ROOMBA_ADDRESSES[4][5] = {
	{0xAA,0xAA,0xAA,0xAA,0xAA},
	{0xBB,0xBB,0xBB,0xBB,0xBB},
	{0xCC,0xCC,0xCC,0xCC,0xCC},
	{0xDD,0xDD,0xDD,0xDD,0xDD}
};

uint8_t ROOMBA_FREQUENCIES [4] = {104, 106, 108, 110};

// each roomba on a differnent freq
// base station
