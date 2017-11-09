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
	init_adc();
}

void test_function(uint16_t data){
	send_reply(ACK_CONNECTION, data);
}

void test_function_2(uint16_t data){
	send_reply(RET_SETTING, data);
}

uint16_t get_temperature() {
	uint8_t adc_value = get_adc_value(0);
	uint16_t temperature = ((adc_value / 255.0 * 5) - 0.5) * 100;
	return temperature;
}

uint16_t get_lux(){
	uint8_t adc_value = get_adc_value(1);
	double ldr_voltage = adc_value / 255.0 * 5;
	double resistor_voltage = 5 - ldr_voltage;
	double ldr_resistance = ldr_voltage / resistor_voltage * 5;
	double lux = 500 / ldr_resistance;
	return lux;
}

void return_status(uint16_t data){
	uint16_t distance = measure_distance();
	send_reply(RET_STATUS, distance);
	send_short_USART(get_lux(), TRANSMIT_LITTLE_ENDIAN); 
}

int main(void){
	setup();
	register_handler(REQ_CONNECTION, test_function);
	register_handler(REQ_SETTING, test_function_2);
	register_handler(REQ_STATUS, return_status);
	
	sei();
 
}

