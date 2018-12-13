#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/portpins.h>
#include <avr/pgmspace.h>

//Usart include files
#include "usart_ATmega1284.h"

//ADC include files
#include "adc.h"

//FreeRTOS include files
#include "FreeRTOS.h"
#include "task.h"
#include "croutine.h"

//nokia includes
#include "glcd.h"
#include "fonts/font5x7.h"

#define A0 (~PINA & 0x01)
#define A1 ((~PINA & 0x02) >> 1)

int totaldistance = 0;
int totalarea = 0;
unsigned char q[10];

enum usartStates {poll}ustate;

enum controlStates{init, startscreen, cleaning }cstate;

void usartTick() {
	static unsigned char temp;
	if (USART_HasReceived(0)) {
		temp = USART_Receive(0);
		totaldistance = temp;
	}
	
}

void controlTick() {
	static unsigned char temp;
	switch (cstate) {
		case init:
			if(A0 | A1) {
				cstate = startscreen;
			} else {
				cstate = init;
			}
			break;
		case startscreen:
			if (A0 | A1) {
				cstate = cleaning;
				glcd_clear_buffer();
				glcd_tiny_draw_string(0,1, "Git-R-done");
				glcd_write();
				if(USART_IsSendReady(0)) {
					USART_Send(2, 0); // 2 for ready random
					while(!USART_HasTransmitted(0));
				}
				delay_ms(100);
			} else {
				cstate = startscreen;
			}
			break;	
		case cleaning:
			if(A0 & A1) {
				cstate = init;
				if(USART_IsSendReady(0)) {
					USART_Send(4, 0); //  4 for reset 
					while(!USART_HasTransmitted(0));
				}
				delay_ms(500);
				glcd_clear_buffer();
				sprintf(q, "%d", totaldistance);
				glcd_tiny_draw_string(0,1,q);
				glcd_tiny_draw_string(0,2, "square meters covered");
				glcd_write();
				delay_ms(3000);
			} else {
				cstate = cleaning;
			}
			break;		
		default:
			cstate = init;
			break;	
	}
	switch (cstate) {
		case init:
			glcd_clear_buffer();
			glcd_tiny_draw_string(0,1, "Press Button   to Start        Cleaning");
			glcd_write();
			break;
		case startscreen:
			glcd_clear_buffer();
			glcd_tiny_draw_string(0,1, "Choose mode:");
			glcd_tiny_draw_string(0,2, "1. Random");
			glcd_write();
			break;	
		case cleaning: 
			glcd_clear_buffer();
			glcd_tiny_draw_string(0,1,"Cleaning in   progress");
			sprintf(q, "%d", totaldistance);
			glcd_tiny_draw_string(0,3, "Distance =");
			glcd_tiny_draw_string(0,4,q);
			glcd_write();
			break;
	}
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void controlTask() {
	cstate = 0;
	for(;;)
	{
		controlTick();
		vTaskDelay(200);
	}
}

void usartTask() {
	ustate = 0;
	for(;;)
	{
		usartTick();
		vTaskDelay(50);
	}
}

void controlPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(controlTask, (signed portCHAR *)"controlTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

void usartPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(usartTask, (signed portCHAR *)"usartTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

int main(void)
{
    DDRA = 0x00; PORTA = 0xFF;
	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0x02; PORTD = 0xFD;
	// initializations
	initUSART(0);
	
	glcd_init();
	glcd_set_contrast(70);
	glcd_tiny_set_font(Font5x7,5,7,32,127);

	/* Replace with your application code */
	//List Tasks
	controlPulse(1);
	usartPulse(2);
	
	//Run scheduler
	vTaskStartScheduler();
}

