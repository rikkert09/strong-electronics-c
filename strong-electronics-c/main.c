#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

#include "rs232.h"
#include "besturings_eenheid_prot.h"
#include "adc.h"

#define F_CPU	16E6

int main(void){
	init_adc();
	init_USART(DEFAULT_UBRR);
	
	uint8_t adc_value = 0;
	while (1)
	{
		adc_value = get_adc_value(4);
		send_byte_USART(adc_value);
		send_byte_USART('\n');
		_delay_ms(20000);
	}
}
