#include <avr/io.h>

#define F_CPU 16E6
#include <util/delay.h>
#include "distance.h"

volatile uint16_t timeout_overflows = 2375;
volatile uint16_t distance = 0;

#define delay_10_us() asm volatile \
("    ldi  r18, 53"	"\n" \
"1:  dec  r18"	"\n"\
"    brne 1b"	"\n"\
"    nop"	"\n");

#define delay_1_us() \
uint8_t counter = 15; \
while(counter)\
counter--;\


void init_distance()
{	
	DDRB = 0xFF;
	DDRB = (1 << PINB0) | (0 << PINB1);
	PORTB = 0x00;
}

void send_pulse(){
	PORTB |= (1 << PINB0);
	delay_10_us();
	PORTB &= ~(1 << PINB0);
}

uint16_t get_distance(){
	return distance;
}

uint16_t measure_distance(){
	send_pulse();
	
	while(!(PINB & _BV(PINB1)));
	uint16_t micro_seconds =  0;
	while(PINB & _BV(PINB1)){
		delay_1_us();
		micro_seconds++;
	}
	
	micro_seconds -= (micro_seconds * (11.0 * (1/16.0))); // subtract loop clock cycles
	distance = micro_seconds / 58;
	return distance;
}
