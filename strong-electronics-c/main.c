#include <avr/io.h>
#define F_CPU 16000000

#include <stdint.h>
#include <string.h>
#include <avr/eeprom.h>

#include "rs232.h"
#include "besturings_eenheid_prot.h"
#include "adc.h"
#include "distance.h"
#include "AVR_TTC_scheduler.h"

#define EXTENDED_PIN PINB5
#define RETRACTED_PIN PINB3
#define MOVING_PIN PINB4

#define LIGHT_MOD

#define SETTING_MIN_EXT_STORAGE (uint16_t*) 2
#define SETTING_MAX_EXT_STORAGE (uint16_t*) 4
#define SETTING_EXTEND_IN_OUT_STORAGE (uint16_t*) 6
#define SENSOR_TRIG_VAL_STORAGE (uint16_t*) 8
#define OP_MODE_STORAGE (uint16_t*) 10
#define EXTENDED_STORAGE (uint16_t*) 12
#define SENSOR_TYPE (uint16_t*) 14

uint16_t sensor_hist [2] = {0, 0};
uint16_t setting_min_ext = 10;
uint16_t setting_max_ext = 130;
#ifdef LIGHT_MOD
uint8_t sensor_type = 1;
uint16_t sensor_trig_val = 150;
#endif
#ifdef TEMP_MOD
uint8_t	sensor_type = 2;
uint16_t sensor_trig_val = 25;
#endif
uint16_t op_mode = OP_MODE_AUTO;
uint16_t setting_ext_in_out = 0;
uint16_t setting_ext_to_val = 0;

uint8_t retracting = 0;
uint8_t extending = 0;
uint8_t extended = 0;

uint16_t sch_extend_index = SCH_MAX_TASKS;
uint16_t sch_retract_index = SCH_MAX_TASKS;

uint8_t current_distance = 0;

uint8_t* device_name = "Rolluik";

uint8_t is_connected = 0;

void check_dist_set_mode();

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
				eeprom_update_word(SETTING_MIN_EXT_STORAGE, new_value);
				succes_failure = SUCCES;
			}
			break;
		case SETTING_MAX_EXTEND:
			if(setting_min_ext < new_value){
				setting_max_ext = new_value;
				eeprom_update_word(SETTING_MAX_EXT_STORAGE, new_value);
				succes_failure = SUCCES;
			}
			break;
		case SETTING_SENSOR_TRIG_VAL:
			sensor_trig_val = new_value;
			eeprom_update_word(SENSOR_TRIG_VAL_STORAGE, new_value);
			succes_failure = SUCCES;
			break;
		case SETTING_OP_MODE:
			op_mode = new_value;
			eeprom_update_word(OP_MODE_STORAGE, new_value);
			succes_failure = SUCCES;
			break;
		case SETTING_EXTEND_IN_OUT:
			setting_ext_in_out = new_value;
			eeprom_update_word(SETTING_EXTEND_IN_OUT_STORAGE, new_value);
			check_dist_set_mode();
			succes_failure = SUCCES;
			break;
		case SETTING_EXTEND_TO_VAL:
			succes_failure = SUCCES;
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
	if(op_mode == OP_MODE_AUTO){
		if(distance <= setting_max_ext && sensor_hist[0] > sensor_trig_val){
			extending = 1;
			PORTB &= ~_BV(RETRACTED_PIN);	//turn of retraced LED
			PORTB |= _BV(EXTENDED_PIN);		//turn on EXTENDED LED
			PORTB ^= _BV(MOVING_PIN);		//toggle moving LED
		}else{
			extending = 0;
			extended = 1;
			sch_extend_index = SCH_Delete_Task(sch_extend_index);
			PORTB &= ~_BV(RETRACTED_PIN);	//turn of retraced LE
			PORTB |= _BV(EXTENDED_PIN);		//keep extended LED lit
			PORTB &= ~_BV(MOVING_PIN);		//turn of moving LED
		}			
	}else if(op_mode == OP_MODE_MANUAL){
		if(distance <= setting_max_ext){
			extending = 1;
			PORTB &= ~_BV(RETRACTED_PIN);	//turn of retraced LED
			PORTB |= _BV(EXTENDED_PIN);		//turn on EXTENDED LED
			PORTB ^= _BV(MOVING_PIN);		//toggle moving LED
		}else{
			extending = 0;
			extended = 1;
			sch_extend_index = SCH_Delete_Task(sch_extend_index);
			PORTB &= ~_BV(RETRACTED_PIN);	//turn of retraced LED
			PORTB |= _BV(EXTENDED_PIN);		//keep extended LED lit
			PORTB &= ~_BV(MOVING_PIN);		//turn of moving LED
		}
	}
}	


void retract(){
	uint16_t distance = measure_distance();
	if(op_mode == OP_MODE_AUTO){
		if(distance > setting_min_ext && sensor_hist[0] < sensor_trig_val){
			retracting = 1;
			PORTB &= ~_BV(EXTENDED_PIN);
			PORTB |= _BV(RETRACTED_PIN);
			PORTB ^= _BV(MOVING_PIN);
			PORTB &= ~_BV(EXTENDED_PIN);
		}else{
			sch_retract_index = SCH_Delete_Task(sch_retract_index);
			retracting = 0;
			extended = 0;
			PORTB |= _BV(RETRACTED_PIN);	//keep extended LED lit
			PORTB &= ~_BV(MOVING_PIN);		//turn of moving LED
		}
	}else if(op_mode == OP_MODE_MANUAL){
		if(distance > setting_min_ext && setting_ext_in_out == 0){
			retracting = 1;
			PORTB &= ~_BV(EXTENDED_PIN);
			PORTB |= _BV(RETRACTED_PIN);
			PORTB ^= _BV(MOVING_PIN);
		}else{
			sch_retract_index = SCH_Delete_Task(sch_retract_index);
			retracting = 0;
			extended = 0;
			PORTB &= ~_BV(EXTENDED_PIN);
			PORTB |= _BV(RETRACTED_PIN);	//keep extended LED lit
			PORTB &= ~_BV(MOVING_PIN);		//turn of moving LED
		}
	}	
}

void check_dist_set_mode(){
	uint16_t distance = measure_distance();
	if(op_mode == OP_MODE_AUTO){
		if(sensor_hist[0] > sensor_trig_val && distance <= setting_max_ext && !extending){
			retracting = 0;
			extending = 1;
			if(sch_retract_index != SCH_MAX_TASKS){
				sch_retract_index = SCH_Delete_Task(sch_retract_index);
			}			
			sch_extend_index = SCH_Add_Task(extend, 1, 4);
		}else if(sensor_hist[0] < sensor_trig_val && (distance >= setting_min_ext) && !retracting){
			extending = 0;
			retracting = 1;
			if(sch_extend_index != SCH_MAX_TASKS){
				sch_extend_index = SCH_Delete_Task(sch_extend_index);
			}			
			sch_retract_index = SCH_Add_Task(retract, 1, 4);
		}
	}else if(op_mode == OP_MODE_MANUAL){
		if(distance <= setting_max_ext && !extending && setting_ext_in_out == 1){
			extending = 1;
			retracting = 0;
			if(sch_retract_index != SCH_MAX_TASKS){
				sch_retract_index = SCH_Delete_Task(sch_retract_index);
			}
			sch_extend_index = SCH_Add_Task(extend, 0, 4);
		}else if(distance >= setting_min_ext && !retracting && setting_ext_in_out == 0){
			extending = 0;
			retracting = 1;
			if(sch_extend_index != SCH_MAX_TASKS){
				sch_extend_index = SCH_Delete_Task(sch_extend_index);
			}
			sch_retract_index = SCH_Add_Task(retract, 0, 4);
		}			
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

void handler_request_sensor_type(uint16_t data){
	uint16_t sensor_type = 0;
	#ifdef LIGHT_MOD
		sensor_type = SENSOR_TYPE_LIGHT;
	#endif
	#ifdef TEMP_MOD
		sensor_type = SENSOR_TYPE_TEMP;
	#endif
	
	send_reply(RET_SENSOR_TYPE, sensor_type);
}

void setup(){
	initalize_control_unit_prot();
	init_distance();
	init_adc();
	
	DDRB |= (1 << RETRACTED_PIN) | (1 << MOVING_PIN) | (1 << EXTENDED_PIN);
	PORTB |= _BV(RETRACTED_PIN);

	register_handler(REQ_DEVICE_NAME, handler_request_device_name);
	register_handler(REQ_CONNECTION, handler_connection_request);
	register_handler(REQ_DISCONNECT, handler_request_disconnect);
	register_handler(REQ_STATUS, return_status);
	register_handler(REQ_SETTING, handler_request_setting);
	register_handler(UPD_SETTING, handler_update_setting);
	register_handler(REQ_SENSOR_TYPE, handler_request_sensor_type);
	
	if(eeprom_read_word(SENSOR_TYPE) != sensor_type){
		write_settings();
	}else {
		setting_ext_in_out = eeprom_read_word(SETTING_EXTEND_IN_OUT_STORAGE);
		setting_min_ext = eeprom_read_word(SETTING_MIN_EXT_STORAGE);
		setting_max_ext = eeprom_read_word(SETTING_MAX_EXT_STORAGE);
		op_mode = eeprom_read_word(OP_MODE_STORAGE);
		sensor_trig_val = eeprom_read_word(SENSOR_TRIG_VAL_STORAGE);
		extended = eeprom_read_word(EXTENDED_STORAGE);
	}

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

void write_settings(){
	eeprom_update_word(SETTING_EXTEND_IN_OUT_STORAGE, setting_ext_in_out);
	eeprom_update_word(SETTING_MIN_EXT_STORAGE, setting_min_ext);
	eeprom_update_word(SETTING_MAX_EXT_STORAGE, setting_max_ext);
	eeprom_update_word(OP_MODE_STORAGE, op_mode);
	eeprom_update_word(SENSOR_TRIG_VAL_STORAGE, sensor_trig_val);
	eeprom_update_word(EXTENDED_STORAGE, extended);
	eeprom_update_word(SENSOR_TYPE, sensor_type);
}

int main(void){
	setup();
	update_status();
	SCH_Start();

    while (1){
		SCH_Dispatch_Tasks();
	}
}
