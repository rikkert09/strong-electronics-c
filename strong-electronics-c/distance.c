#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 16E6
#include <util/delay.h>
#include "distance.h"

volatile uint16_t overflow_counter = 0;
volatile uint8_t sensor_activated = 0;
volatile uint16_t distance = 0;
volatile uint16_t timeout_overflows = 2375;

void init_distance()
{	
	DDRB = 0xFF;
	DDRD = 0x00;
	PORTB = 0x00;
	
	TCNT0 = 0;
	TIMSK0 |= (1<<TOIE0);
	
	sei();
}

void send_pulse(){
	PORTB = 0xFF;
	_delay_us(10);
	PORTB = 0x00;
	_delay_us(10);
}

uint16_t get_distance(){
	return distance;
}

uint16_t measure_distance(){
	_delay_ms(200);
	send_pulse();
	while(!(PIND & _BV(PIND2)));
	TCNT0 = 0;
	TCCR0B |= (1<<CS01);
	overflow_counter = 0;
	sensor_activated = 1;
	while(PIND & _BV(PIND2));
	TCCR0B = 0;
	distance = ((overflow_counter) * 128 + (TCNT0 / 2)) / 58;
	return distance;
}

ISR(TIMER0_OVF_vect)
{
	if(sensor_activated){
		overflow_counter++;
		if(overflow_counter >= timeout_overflows){
			TCCR0B = 0;
			distance = 0;
			sensor_activated = 0;
		}
	}
}
