#include <avr/io.h>
#define F_CPU 16000000

#include <stdint.h>

#include "rs232.h"
#include "besturings_eenheid_prot.h"
#include "adc.h"
#include "distance.h"
#include "AVR_TTC_scheduler.h"

#define LIGHT_MOD

uint16_t sensor_hist [2] = {0, 0};

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
	//uint16_t distance = measure_distance();
	uint16_t avg_sensor = (sensor_hist[0] + sensor_hist[1])/2;
	send_reply(RET_STATUS, (uint16_t)0x1000);
	send_short_USART(avg_sensor, TRANSMIT_LITTLE_ENDIAN);
}

void update_status(){
	sensor_hist[1] = sensor_hist[0];
#ifdef LIGHT_MOD
	sensor_hist[0] = get_lux();
#endif
#ifdef TEMP_MOD
	sensor_hist[0] = get_temperature();
#endif
}

void setup(){
	initalize_control_unit_prot();
	//init_distance();
	init_adc();
	
	register_handler(REQ_STATUS, return_status);
	
	SCH_Init_T1();	
	SCH_Add_Task(handle_comms, 0, 1);
	#ifdef LIGHT_MOD
	SCH_Add_Task(update_status, 300, 300);				// if this is a light sensor, update every 30 sec
	#endif
	#ifdef TEMP_MOD
	SCH_Add_Task(update_status, 400, 400);				// if this is a temperature sensor, update every 40 sec
	#endif
}

int main(void){
	setup();
	SCH_Start();
	
    while (1){
		SCH_Dispatch_Tasks();
	}
}

