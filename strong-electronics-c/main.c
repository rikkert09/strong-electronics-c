#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

#include "rs232.h"
#include "besturings_eenheid_prot.h"

int main(void){

    void setup(){
        //init_USART(DEFAULT_UBRR);
        DDRD = 0xff;
        PORTD = 0x00;
    }

    int main(void){
        setup();

        while (1){
            PORTD = 0xFF;
            _delay_ms(1000);
            PORTD = 0x00;
            _delay_ms(1000);
        }
    }
}