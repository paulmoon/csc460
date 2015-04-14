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

#define GAME_ZOMBIE 0
#define GAME_HUMAN  1
typedef enum _roomba_nums {PLAYER1 = 0,PLAYER2, PLAYER3,PLAYER4 } GAME_PLAYERS;
// typedef enum _roomba_nums {COP1 = 0, COP2, ROBBER1, ROBBER2} COPS_AND_ROBBERS;
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

// ---------------
// GAME PACKETS
// ---------------
typedef enum _game_packet_types {
    GAME_ARE_YOU_READY, // First packet sent by base to tell the roombas to ready

    GAME_START,         // Once all roombas are ready. This command officially starts
                        // the game. The roombas should then move into a state where
                        // they can start receving commands from the players

    GAME_PROGRESS,  // sent from the roomba to the base station whenever the
                    // roomba gets hit

    GAME_UPDATE,    // sent from the base station to the roombas

    GAME_OVER,      // Once a win/lose condition is met. This tells the roombas
                    // to stop

    GAME_ACK        // ACK packet used to tell the base station if a packet was
                    // received.
} GAME_PACKET_TYPES;


struct game_player_state {
    int8_t alliance;        // ZOMBIE,HUMAN
    int8_t roomba_id;       // PLAYER1,PLAYER2,PLAYER3,PLAYER4
};

struct game_are_you_ready{};

struct game_start{
    struct game_player_state states[4];
};

struct game_progress{
    int8_t hit;         // 0 not hit, 1 is hit
    int8_t roomba_id;   // the id of the roomba that got hit
    int8_t enemy_id;    // who hit me.
};

struct game_update{
	struct game_player_state states[4];
};

struct game_over {
    int8_t winning_team;
};

struct game_ack{
    int8_t sender_address[5];   // address of the sender
    int8_t roomba_id;           // roomba_id of the sender
    int8_t ack_num;             // the seq number being acked.
};

// Can only fill up to 28 bytes
typedef struct _game_data {
    int8_t seq_num;
    union {
        // max size is 27 bytes
        struct game_are_you_ready are_you_ready;
        struct game_start start;
        struct game_progress progress;
        struct game_update update;
        struct game_over over;
        struct game_ack ack;
    };
} game_data_t;


#endif /* COPS_AND_ROBBERS_H_ */