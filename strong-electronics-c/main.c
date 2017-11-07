#include <avr/io.h>
#define F_CPU 16000000

#include <util/delay.h>
#include <stdint.h>
#include <avr/interrupt.h>

#include "rs232.h"
#include "besturings_eenheid_prot.h"
#include "adc.h"
#include "distance.h"


void setup(){
	initalize_control_unit_prot();
	init_distance();
}

void test_function(uint16_t data){
	send_reply(ACK_CONNECTION, data);
}

void test_function_2(uint16_t data){
	send_reply(RET_SETTING, data);
}

void return_status(uint16_t data){
	uint16_t distance = measure_distance();
	send_reply(RET_STATUS, distance);
	send_short_USART(0, TRANSMIT_LITTLE_ENDIAN);
}

int main(void){
	setup();
	register_handler(REQ_CONNECTION, test_function);
	register_handler(REQ_SETTING, test_function_2);
	register_handler(REQ_STATUS, return_status);
	
	sei();

    while (1);
}

