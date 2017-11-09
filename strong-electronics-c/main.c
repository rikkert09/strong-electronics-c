#include <avr/io.h>
#define F_CPU 16000000

#include <stdint.h>
#include <string.h>

#include "rs232.h"
#include "besturings_eenheid_prot.h"
#include "adc.h"
#include "distance.h"
#include "AVR_TTC_scheduler.h"

#define LIGHT_MOD

uint16_t sensor_hist [2] = {0, 0};
uint16_t setting_min_ext = 10;
uint16_t setting_max_ext = 130;
uint16_t sensor_trig_val = 150;
uint16_t op_mode = OP_MODE_AUTO;
uint16_t setting_ext_in_out = 0;
uint16_t setting_ext_to_val = 0;

uint8_t* device_name = "Dummy Name";

uint8_t is_connected = 0;

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

void handler_connection_request(uint16_t data){
	is_connected = 1;
	send_reply(ACK_CONNECTION, 0);
}

void handler_request_device_name(uint16_t data){
	send_byte_USART(RET_DEVICE_NAME);
	write_string(device_name);
}

void handler_update_device_name(uint16_t data){
	uint8_t *new_name = read_string(new_name, data);
	strcpy(device_name, new_name);
}

void handler_request_setting(uint16_t data){
	uint16_t ret_val = 0;
	switch(data){
		case SETTING_MIN_EXTEND:
			ret_val = setting_min_ext;
			break;
		case SETTING_MAX_EXTEND:
			ret_val = setting_max_ext;
			break;
		case SETTING_SENSOR_TRIG_VAL:
			ret_val = sensor_trig_val;
			break;
		case SETTING_OP_MODE:
			ret_val = op_mode;
			break;
		case SETTING_EXTEND_IN_OUT:
			ret_val = setting_ext_in_out;
			break;
		case SETTING_EXTEND_TO_VAL:
			ret_val = setting_ext_to_val;
			break;
	}

	send_reply(RET_SETTING, ret_val);
}

void handler_update_setting(uint16_t data){
	uint8_t succes_failure = FAILURE;
	uint8_t setting = data >> 8	;
	uint8_t new_value = data;
	switch(setting){
		case SETTING_MIN_EXTEND:
			if(new_value < setting_max_ext){
				setting_min_ext = new_value;
				succes_failure = SUCCES;
			}
			break;
		case SETTING_MAX_EXTEND:
			if(new_value > setting_min_ext){
				setting_max_ext = new_value;
				succes_failure = SUCCES;
			}
			break;
		case SETTING_SENSOR_TRIG_VAL:
			sensor_trig_val = new_value;
			succes_failure = SUCCES;
			break;
		case SETTING_OP_MODE:
			op_mode = new_value;
			succes_failure = SUCCES;
			break;
		case SETTING_EXTEND_IN_OUT:
			break;
		case SETTING_EXTEND_TO_VAL:
			break;
	}

	send_reply(ACK_UPD_SETTING, succes_failure);
}

void handler_request_disconnect(uint16_t data){
	is_connected = 0;
	send_reply(ACK_DISCONNECT, 0);
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

	register_handler(REQ_DEVICE_NAME, handler_request_device_name);
	register_handler(REQ_CONNECTION, handler_connection_request);
	register_handler(REQ_DISCONNECT, handler_request_disconnect);
	register_handler(REQ_STATUS, return_status);
	register_handler(REQ_SETTING, handler_request_setting);
	register_handler(UPD_SETTING, handler_update_setting);

	SCH_Init_T1();

	SCH_Add_Task(handle_comms, 0, 1);
#ifdef LIGHT_MOD
	SCH_Add_Task(update_status, 250, 300);				// if this is a light sensor, update every 30 sec
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

