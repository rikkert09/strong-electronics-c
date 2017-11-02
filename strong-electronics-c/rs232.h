#ifndef STRONG_ELECTRONICS_C_RS232_H
#define STRONG_ELECTRONICS_C_RS232_H
#define FOSC 16E6 // Clock Speed#define BAUD 9600#define DEFAULT_UBRR FOSC/16/BAUD-1

#define TRANSMIT_LITTLE_ENDIAN 0
#define TRANSMIT_BIG_ENDIAN 1

void init_USART(uint16_t ubrr);

void send_byte_USART(uint8_t data);

void send_short_USART(uint16_t data, uint8_t endiannes);

uint8_t receive_byte_USART(void);

uint16_t receive_short_USART(uint8_t endiannes);


#endif //STRONG_ELECTRONICS_C_RS232_H
