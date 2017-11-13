#ifndef STRONG_ELECTRONICS_C_BESTURINGS_EENHEID_PROT_H
#define STRONG_ELECTRONICS_C_BESTURINGS_EENHEID_PROT_H

//COMMAND_BYTES
#define REQ_CONNECTION              0x3F
#define ACK_CONNECTION              0xFF

#define REQ_DEVICE_NAME             0x02
#define RET_DEVICE_NAME             0x82

// send new device name
#define UPD_DEVICE_NAME             0x03

// confirm update of new device name
#define CON_NEW_DEVICE_NAME 0x83

#define REQ_SENSOR_TYPE             0x04
#define RET_SENSOR_TYPE             0x84

#define REQ_STATUS                  0x08
#define RET_STATUS                  0x88

#define REQ_SETTING                 0x10
#define RET_SETTING                 0x90

#define UPD_SETTING                 0x20
#define ACK_UPD_SETTING             0xA0

#define REQ_DISCONNECT              0x00
#define ACK_DISCONNECT              0x80

// possible sensor types
#define SENSOR_TYPE_TEMP            0x0000
#define SENSOR_TYPE_LIGHT           0x0100

// possible settings to request and update
#define SETTING_MIN_EXTEND          0x0000
#define SETTING_MAX_EXTEND          0x0001
#define SETTING_SENSOR_TRIG_VAL     0x0002
#define SETTING_OP_MODE             0x0003
#define SETTING_EXTEND_IN_OUT       0x0004
#define SETTING_EXTEND_TO_VAL       0x0005

#define OP_MODE_AUTO                0x0000
#define OP_MODE_MANUAL              0x0001

#define SUCCES                      0x00
#define FAILURE                     0xFF

void initalize_control_unit_prot();

uint8_t receive_command();

void send_reply(uint8_t message, uint16_t data);

uint8_t register_handler(uint8_t, void (*)(uint16_t));

uint8_t* read_string(uint8_t  *dest, uint8_t len);
void write_string(uint8_t *string);

void handle_comms();

#endif //STRONG_ELECTRONICS_C_BESTURINGS_EENHEID_PROT_H
