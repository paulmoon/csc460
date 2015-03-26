#include <avr/io.h>
#include <avr/interrupt.h>
#include "analog.h"

// Digital Pin 10 should be attached to an LED
// #define ANALOG_DEBUG_INIT (DDRB |= (1 << PB4));
// #define ANALOG_DEBUG_ON (PORTB |= (1<<PB4));
// #define ANALOG_DEBUG_OFF (PORTB &= ~(1<<PB4));

// Setup in free running mode
// prescalar 128 giving us 125KHz sample rate
// Default analog pin 0
// ADC Reference to AVCC
void Analog_init()
{
    // Set ADC prescalar to 128 - 125KHz sample rate @ 16MHz (p 281)
    ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

    ADMUX = 0;
    ADMUX |= (1 << REFS0); // Set ADC reference to AVCC
    //ADMUX |= (1 << ADLAR); // Left adjust ADC result to allow easy 8 bit reading
    // No MUX5:0 bits need to be changed to use ADC0

    // ADCSRB
    ADCSRB = 0;
    // free running mode is default

    //ADCSRA |= (1 << ADIE);  // Enable the ADC interrupt handler
    ADCSRA = 0;
    ADCSRA |= (1 << ADEN);  // Enable ADC
    ADCSRA |= (1 << ADSC);  // Start A2D Conversions for initialization

    // wait until the first conversion is done.
    while( ADCSRA & (1<<ADSC) );
}


/*
    helper function to setup the mux to read from the
    correct analog ping.
    See p282 for full details on how to use selection.
*/
static void _setup_pin(uint8_t pin_number){

    ADMUX &= 0xE0;	// clear the MUX4:0 bits
    ADMUX |= pin_number&0x07; // set to read from the correct pin number
	ADCSRB = pin_number&(1<<3); // set the MUX5 bit if necessary
}

/*
    read the analog value from the specified pin number
*/
uint16_t Analog_read(uint8_t pin_number)
{
    // set the target pin to be read from
    _setup_pin(pin_number);

    // kick off a read/conversion from the pin
    ADCSRA |= (1 << ADSC);
    // pause until the ADCSRA is done its conversion before trying to read
    while( ADCSRA & (1<<ADSC) );

    // important that we read the low byte first and then we read the high byte
    uint16_t low = ADCL;
    uint16_t high = ADCH;
    return ((high << 8) | low);
}

/*
    Read all the values of each analog pin.
    Place the result into the specific index into the
    passed in array.
*/
void Analog_poll(uint16_t* output_values)
{
    if( output_values == 0){return;}
    uint16_t low,high;

    for(int i = 0; i < ANALOG_NUM_PINS; ++i)
    {
        // select pin 'i' to read from
        _setup_pin(i);

        // kick off conversion; then wait until its done
        ADCSRA |= ( 1 << ADSC);
        while( ADCSRA & (1<<ADSC) );

        // read low byte, then high byte
        low = ADCL;
        high = ADCH;

        // store the read value into the output array
        output_values[i] = ((high << 8) | low);
    }
}

/*
User defined function which is passed in the latest read value
*/
// extern void Analog_read_handler(uint16_t value);
// ISR(ADC_vect) {
//     uint16_t low = ADCL;
//     uint16_t high = ADCH;
//     Analog_read_handler((high << 8) | low);
// }