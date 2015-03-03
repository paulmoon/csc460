#ifndef __UART_H__
#define __UART_H__
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

/** modifications
	- moved the volatile uint8_t uart_rx variable from the h file to the c file
	- renamed to cpp file
	- changed the cli() and sei() functions to Disable_interrupts() and Enable_Interrupts()	
	- removed UART_BPS code and hard-coded it to be 9600 baud
*/

#define UART_BUFFER_SIZE 100			// size of Rx ring buffer.

void uart_init();
void uart_putchar(char c);
char uart_getchar(int index);
void uart_putstr(char *s);
int uart_write(uint8_t* const str, int len);

uint8_t uart_bytes_recv(void);
void uart_reset_recv(void);
#endif
