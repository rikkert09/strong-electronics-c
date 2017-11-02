#include <avr/io.h>

#define F_CPU 16e6
#include <util/delay.h>

int main(void)
{
	DDRD = 0xff;
	PORTD = 0x00;
    while(1)
    {
        PORTD = 0xFF;
		_delay_ms(1000);
		PORTD = 0x00;
		_delay_ms(1000);
    }
}