#include <avr/io.h>
#include "timer.h"
#include "glcd.h"
#include "adc.h"
#include "fonts/font5x7.h"
#include "usart_ATmega1284.h"

#define C0 (~PINC & 0x01)

typedef struct _task {
	/*Tasks should have members that include: state, period,
		a measurement of elapsed time, and a function pointer.*/
	signed char state; //Task's current state
	unsigned long int period; //Task period
	unsigned long int elapsedTime; //Time elapsed since last task tick
	int (*TickFct)(int); //Task tick function
} task;

enum statesADC {poll};
int ADCsm(int state) {
	static unsigned short a = 0;
	static unsigned char q[6];
	
	switch (state) {
		case poll:
			state = poll;
			break;
		default:
			state = poll;
	}
	
	switch (state) {
		case poll:
		glcd_clear_buffer();
		
		setADC(1);
		a = (unsigned short)ADC;
		sprintf(q, "%d", a);
		glcd_tiny_draw_string(1,1,q);
		delay_ms(1);
		
		setADC(2);
		a = (unsigned short)ADC;
		sprintf(q, "%d", a);
		glcd_tiny_draw_string(1,2,q);
		delay_ms(1);
		
		setADC(3);
		a = (unsigned short)ADC;
		sprintf(q, "%d", a);
		glcd_tiny_draw_string(1,3,q);
		delay_ms(1);
		
		glcd_write();
			break;
	}
	return state;
}


enum usartMsm {send};
int usartM(int state) {
	unsigned char temp;
	switch (state) {
		case send:
			state = send;
			break;
		default:
			state = send;
			break;	
	}
	switch (state) {
		case send:
			if(USART_IsSendReady(1)) {
				temp = C0;
				PORTC |= (temp & 0x01) << 4;
				USART_Send(C0, 1);
				while(!USART_HasTransmitted(1));
			}
			break;
	}
	
	return state;
}

enum motorsm {init, cw};
int motor (int state) {
	unsigned char temp = 0x01;
	switch (state) {
		case init:
			state = cw;
			PORTC &= 0x9F;
			PORTC |= (temp & 0x03) << 5;
			break;
		case cw:
			if (C0) {
				state = cw;
				temp = !temp;
				PORTC |= (temp & 0x03) << 5;
			} else {
				state = cw;
			}
			break;
	}
	return state;	
}

int main(void)
{
	DDRA = 0x00; PORTB = 0xFF;
	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0xF0; PORTC = 0x0F;
	DDRD = 0x02; PORTD = 0xFD;
    /* Replace with your application code */
	
	static task task1, task2;
	task *tasks[] = { &task1, &task2};
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);

	// Task 1
	task1.state = 0;//Task initial state.
	task1.period = 50 ;//Task Period.
	task1.elapsedTime = 0;//Task current elapsed time.
	task1.TickFct = &ADCsm;
	
	task2.state = 0;//Task initial state.
	task2.period = 200;//Task Period.
	task2.elapsedTime = 0;//Task current elapsed time.
	task2.TickFct = &usartM;
	
	task2.state = 0;//Task initial state.
	task2.period = 200;//Task Period.
	task2.elapsedTime = 0;//Task current elapsed time.
	task2.TickFct = &motor;
	
	unsigned char GCD = 50;
	
	initUSART(1);

	TimerSet(GCD);
	TimerOn();
	
	ADC_init();
	glcd_init();
	glcd_set_contrast(70);
	glcd_tiny_set_font(Font5x7,5,7,32,127);
	
	
	unsigned char i;
    while (1) 
    {
			// Scheduler code
		for ( i = 0; i < numTasks; i++ ) {
			
			// Task is ready to tick
			if ( tasks[i]->elapsedTime >= tasks[i]->period ) {
				// Setting next state for task
				//PORTA = tasks[i]->state;
				tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
				
				//PORTA = tasks[i]->state;
				// Reset the elapsed time for next tick.
				tasks[i]->elapsedTime = 0;
			}
			tasks[i]->elapsedTime += GCD;
		}
		//PORTA = receivedData;
		while(!TimerFlag);
		TimerFlag = 0;
	}
}