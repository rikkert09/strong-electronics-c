#include <avr/io.h>
#define F_CPU 16000000

#include <stdint.h>
#include <string.h>

#include "rs232.h"
#include "besturings_eenheid_prot.h"
#include "adc.h"
#include "distance.h"
#include "AVR_TTC_scheduler.h"

#define EXTENDED_PIN PINB5
#define RETRACTED_PIN PINB3
#define MOVING_PIN PINB4

#define LIGHT_MOD

uint16_t sensor_hist [2] = {0, 0};
uint16_t setting_min_ext = 10;
uint16_t setting_max_ext = 130;
uint16_t sensor_trig_val = 150;
uint16_t op_mode = OP_MODE_AUTO;
uint16_t setting_ext_in_out = 0;
uint16_t setting_ext_to_val = 0;

extern uint16_t distance;

uint8_t retracting = 0;
uint8_t extending = 0;

uint16_t sch_extend_index = SCH_MAX_TASKS;
uint16_t sch_retract_index = SCH_MAX_TASKS;

uint8_t current_distance = 0;

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
	send_reply(RET_STATUS, measure_distance());
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
			if(setting_min_ext < new_value){
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

void extend(){
	uint16_t distance = measure_distance();
	if(distance <= setting_max_ext && sensor_hist[0] > sensor_trig_val){
		extending = 1;
		PORTB &= ~_BV(RETRACTED_PIN);	//turn of retraced LED
		PORTB |= _BV(EXTENDED_PIN);		//turn on EXTENDED LED
		PORTB ^= _BV(MOVING_PIN);		//blink moving LED
	}else{
		setting_ext_in_out = 1;
		extending = 0;
		sch_extend_index = SCH_Delete_Task(sch_extend_index);
		PORTB |= _BV(EXTENDED_PIN);		//keep extended LED lit
		PORTB &= ~_BV(MOVING_PIN);		//turn of moving LED
	}
}

void retract(){
	uint16_t distance = measure_distance();
	if(distance > setting_min_ext && sensor_hist[0] < sensor_trig_val){
		retracting = 1;
		PORTB &= ~_BV(EXTENDED_PIN);
		PORTB |= _BV(RETRACTED_PIN);
		PORTB ^= _BV(MOVING_PIN);
	}else{
		sch_retract_index = SCH_Delete_Task(sch_retract_index);
		setting_ext_in_out = 0;
		retracting = 0;
		PORTB |= _BV(RETRACTED_PIN);	//keep extended LED lit
		PORTB &= ~_BV(MOVING_PIN);		//turn of moving LED
	}	
}

void check_dist_set_mode(){
	uint16_t distance = measure_distance();
	if(sensor_hist[0] > sensor_trig_val && distance < setting_max_ext && !extending){
		setting_ext_in_out = 1;
		sch_extend_index = SCH_Add_Task(extend, 0, 4);
	}else if(sensor_hist[0] < sensor_trig_val && distance > setting_min_ext && !retracting){
		setting_ext_in_out = 0;
		sch_retract_index = SCH_Add_Task(retract, 0, 4);
	}
}
	
void update_status(){
	sensor_hist[1] = sensor_hist[0];
#ifdef LIGHT_MOD
	sensor_hist[0] = get_lux();
#endif
#ifdef TEMP_MOD
	sensor_hist[0] = get_temperature();
#endif
	check_dist_set_mode();
}

void setup(){
	initalize_control_unit_prot();
	init_distance();
	init_adc();
	
	DDRB |= 0b00111000;
	PORTB |= _BV(RETRACTED_PIN) | _BV(EXTENDED_PIN) | _BV(MOVING_PIN);

	register_handler(REQ_DEVICE_NAME, handler_request_device_name);
	register_handler(REQ_CONNECTION, handler_connection_request);
	register_handler(REQ_DISCONNECT, handler_request_disconnect);
	register_handler(REQ_STATUS, return_status);
	register_handler(REQ_SETTING, handler_request_setting);
	register_handler(UPD_SETTING, handler_update_setting);

	SCH_Init_T1();

	SCH_Add_Task(handle_comms, 10, 1);
	SCH_Add_Task(check_dist_set_mode, 0, 0);
	#ifdef LIGHT_MOD
	SCH_Add_Task(update_status, 250, 50);				//TODO if this is a light sensor, update every 30 sec
	#endif
	#ifdef TEMP_MOD
	SCH_Add_Task(update_status, 400, 400);				// if this is a temperature sensor, update every 40 sec
	#endif
}

int main(void){
	setup();
	update_status();
	SCH_Start();

    while (1){
		SCH_Dispatch_Tasks();
	}
}

