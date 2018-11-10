//Test Bluetooth Slave 


#include <avr/io.h>
#include "usart_ATmega1284.h"
#include "timer.h"

#define A0 (PINA & 0x01)

unsigned char temp;

enum states {init, waiting}state ;
	
void usartS() {
	switch (state)
	{
	case init:
		USART_Flush(0);
		state = waiting;
		break;
	case waiting:
		if(USART_HasReceived(0)) {
			temp = USART_Receive(0);
			if(temp != 0) {
				PORTA = temp;
			} else {
				PORTA = 0;
			}
			
			state = waiting;
		} else {
			PORTA = 0;
		}
		break;	
		
	default:
		state = init;	
	
	}

	
}

int main(void)
{

	DDRA = 0xFF; PORTA = 0x00;
	DDRD = 0x02; PORTD = 0xFD;
	
	initUSART(0);	

	TimerOn();
	TimerSet(100);
		
    while (1) {
		usartS();
		while(!TimerFlag) {
		}
		TimerFlag = 0;
		
		
    }
}

