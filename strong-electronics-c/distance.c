#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 16E6
#include <util/delay.h>
#include "distance.h"
#include "rs232.h"

volatile uint16_t overflow_counter = 0;
volatile uint8_t sensor_activated = 0;
volatile uint8_t distance_measuring = 0;
volatile uint16_t distance = 0;

void init_distance()
{	
	DDRB = 0xFF;
	DDRD = 0x00;
	PORTB = 0x00;
	
	EICRA = (1 << ISC10);
	EIMSK = (1 << INT1);
	
	TCNT0 = 0;
	TIMSK0 |= (1<<TOIE0);
	
	sei();
}

void send_pulse(){
	PORTB = 0xFF;
	distance_measuring = 1;
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
	while(distance_measuring == 1);
	return distance;
}

ISR (INT1_vect)
{
	if(distance_measuring == 1){
		if(sensor_activated == 0){
			TCNT0 = 0;
			TCCR0B |= (1<<CS00);
			overflow_counter = 0;
			sensor_activated = 1;
		}
		else{
			sensor_activated = 0;
			TCCR0B = 0;
			distance = ((overflow_counter) * 16 + (TCNT0 / 16)) / 58;
			distance_measuring = 0;
		}
	}
}

ISR(TIMER0_OVF_vect)
{
	if(sensor_activated){
		overflow_counter++;
	}
}
