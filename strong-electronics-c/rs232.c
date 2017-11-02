#include <avr/io.h>
#include "rs232.h"

/**
 * Initialize the USART transmitter and receiver with 8 bit data and 2 stop bits
 * in asynchronous mode
 * @param ubrr clockcycles per baud tick
 */
void init_USART(uint16_t ubrr){
    // ubrr = usart baud rate register

    // set the baud rate
    UBRR0H = (uint8_t ) ubrr >> 8;
    UBRR0L = (uint8_t ) ubrr;

    UCSR0B |= (1 << TXEN0) | (1 << RXEN0);  // enable receive + transmit
    UCSR0C |= (3 << UCSZ00) | (1 << USBS0); // 8 data bit; no parity; 2 stop bi
}

/**
 * Send a single byte via USART
 * @param data
 */
void send_byte_USART(uint8_t data){
    while ( !(UCSR0A & (1 << UDRE0)));      // wait till buffer transmit buffer is empty
    UDR0 = data;                            // put data in buffer
}

/**
 * send 2 bytes of data via USART in
 * @param data data to be sent
 * @param endiannes the endiannes is which data is transmitted
 */
void send_short_USART(uint16_t data, uint8_t endiannes){
    if(endiannes == TRANSMIT_LITTLE_ENDIAN){
        send_byte_USART((uint8_t) data >> 8);
        send_byte_USART((uint8_t) data);
    } else {
        send_byte_USART((uint8_t) data);
        send_byte_USART((uint8_t) data >> 8);

    }
}

/**
 * receive 1 byte of data via USART
 * @return received byte of data
 */
uint8_t receive_byte_USART(void){
    while ( !(UCSR0A & (1 << RXC0)));       // wait till receive buffer is full
    return UDR0;                            // return data from buffer
}

/**
 * Receive a short in given endiannes
 * @param endiannes
 * @return received value
 */
uint16_t receive_short_USART(uint8_t endiannes){
    uint16_t received = 0;
    if (endiannes == TRANSMIT_LITTLE_ENDIAN) {      // receive low byte first
        received = receive_byte_USART() << 8;
        received |= receive_byte_USART();
    } else if (endiannes == TRANSMIT_BIG_ENDIAN){   // receive high byte first
        received = receive_byte_USART();
        received |= received << 8;
    }

    return received;
}