#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>

#include <alloca.h>

#include "rs232.h"
#include "besturings_eenheid_prot.h"
#include "list.h"

/************************************************************************/
/* Holds the command number and its handler function                    */
/************************************************************************/
typedef struct {
	uint8_t command;
	void (* pHandlerFunction)(uint16_t);
} command_handler;

/************************************************************************/
/* Will hold a list of all fHandlers. This is a doubly linked list      */
/************************************************************************/
list_t* handler_list = NULL;

void initalize_control_unit_prot(){
	init_USART(DEFAULT_UBRR);		// initialize serial comms
	UCSR0B |= 1 << RXCIE0;			// enable rx interrupt
	
	handler_list = list_new();
}

/************************************************************************/
/* read a command from the serial interface                             */
/************************************************************************/ 
uint8_t receive_command(){
	return receive_byte_USART();
}


/************************************************************************/
/* send a reply message and data                                        */
/************************************************************************/
void send_reply(uint8_t message, uint16_t data){
	send_byte_USART(message);
	send_short_USART(data, TRANSMIT_LITTLE_ENDIAN);
}

/************************************************************************/
/* write a string to UART, terminated by a '\n' character                */
/************************************************************************/
void write_string(uint8_t *string){
	uint8_t i = 0;
	for(i = 0; i < strlen(string); i++){
		send_byte_USART(string[i]);
	}
	send_byte_USART('\n');
}

/************************************************************************/
/* Read a string from USART. The string will be stored at dest and has	*/
/* length len                                                           */
/************************************************************************/
uint8_t* read_string(uint8_t  *dest, uint8_t len){
	uint8_t char_num = 0;
	for(char_num = 0; char_num < len; char_num ++){
		dest[char_num] = receive_byte_USART();
	}
	dest[len] = 0x00;
	return dest;
}

/************************************************************************/
/* Register a command handler, for a given command						*/
/* @param command the code of the command to handle						*/
/* @param pFHandler the callback function to handle the command			*/
/* @return the status of the operation									*/
/*		0 if successful													*/
/*		1 if memory allocation failed									*/
/*		2 if there already is a handler registered                      */
/************************************************************************/
uint8_t register_handler(uint8_t command, void (*pFHandler)(uint16_t)){
	command_handler *new_handler = calloc(sizeof(command_handler), 1);
	list_node_t *new_handler_node = LIST_MALLOC(sizeof(list_node_t));
	
	// Check whether calloc succeeded, return error when not;
	if(new_handler == NULL){
		return 1;
	}
	
	// create a handler
	new_handler -> pHandlerFunction = pFHandler;
	new_handler -> command = command;
	
	//insert handler in list of handlers
	new_handler_node = list_node_new(new_handler_node);
	new_handler_node -> val = new_handler;
	list_rpush(handler_list, new_handler_node);
	return 0;
}

// select the correct handler for the incoming message
// if the message is unknown ignore it, and continue operation
ISR(USART_RX_vect){
	uint8_t rec  = receive_command();
	
	list_node_t *rover = handler_list -> head;
	
	while(rover -> next != handler_list -> head){
		
		command_handler *handler = rover -> val;
		
		if(handler -> command == rec)
		{
			uint16_t command_arg = receive_short_USART(TRANSMIT_LITTLE_ENDIAN);
			handler -> pHandlerFunction(command_arg);
			break;
		}
		
		rover = rover -> next;
	}
}