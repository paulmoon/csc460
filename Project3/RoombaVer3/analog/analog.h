#ifndef _ANALOG_H_
#define _ANALOG_H_

#define ANALOG_NUM_PINS 16

void Analog_init();
uint16_t Analog_read(uint8_t pin_number);
void Analog_poll(uint16_t output_values[16]);

#endif

/*
References:
From Paul Hunter
https://bennthomsen.wordpress.com/arduino/peripherals/analogue-input/
http://www.avrfreaks.net/forum/tut-c-newbies-guide-avr-adc?name=PNphpBB2&file=viewtopic&t=56429

ADMUX - ADC multiplexer selection register
    REFS2,REFS1
    00 - AREF, Internal Vref turned off
    01 - AVCC from external capacitor at the AREF pin
    10 - internal 1.1V
    11 - internal 2.56V
    ADLAR  - left adjust
        read 8 bits from the ADCH register instead of splitting the 10bits
        between the ADCH and ADCL registers
    MUX4,MUX3,MUX2,MUX1,MUX0
        Use this to pick one of the 0-15 analog pins

ADCSRA - ADC control and status register A
    ADEN - ADC Enable
         1 to turn the ADC on, 0 off
    ADSC - ADC Start Conversion
        Set this to 1 to begin a 'conversion'( i.e read)
        first conversion intializes the ADC
        ADSC will read 1 a long as the conversion is in progress.
        When complete it will read 0
    ADATE - ADC Auto Trigger Update
        If set to 1, will auto start a conversion when a positive edge
        is deteted.
    ADIF - ADC Interrupt Flag
        Cleared when writing 1 to the flag.
    ADIE - ADC Interrup Enable
        Enable the ADC conversion complete interrupt
        i.e ISR(ADC) {...}
    ADPS2:0 - ADC Prescalar Select bits
        division factor of the XTAL freq and input clock to the ADC
    000 - 2 division factor
    001 - 2 division factor
    010 - 4
    011 - 8
    100 - 16
    101 - 32
    110 - 64
    111 - 128

ADCSRB - ADC control and status register B
    -,ACME,-,-,MUX5,ADTS2,ADTS1,ADTS0
    ACME - ??
    MUX5 - Use this with the MUX4:0 bits in ADMUX in order to
        select the appropriate analog pin to read from
    ADTS2:0 - ??

ADCL - ADC Data register Low Byte
ADCH  - ADC Data retgister High Byte
    Note you should read first ADCL and ADCH ( if you haven't dont left adjust)

DIDR0 -
    ADC7:0D - ADC Digital Input Disable bits
    When written to logical 1, the digital input buffer corresponding to the
    pin is disabled. Corresponds to ping ADC0:7
DIDR2
    ADC15:8D - ADC Digital Input Disable bits
    When written to logical 1, the digital input buffer corresponding to the
    pin is disabled. Corresponds to ping ADC15:8
*/
