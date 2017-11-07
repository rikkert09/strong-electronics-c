#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

#include "rs232.h"
#include "besturings_eenheid_prot.h"
#include "adc.h"
#include "distance.h"

#define F_CPU	16E6

uint16_t dist = 0;

int main(void){
	init_distance();
	init_USART(DEFAULT_UBRR);
	
	while(1){
		dist = measure_distance();
		send_short_USART(dist, TRANSMIT_LITTLE_ENDIAN);
	}
}
