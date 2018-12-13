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

void delay_ms(int miliSec) {//for 8 Mhz crystal
	int i,j;
	for(i=0;i<miliSec;i++)
	for(j=0;j<775;j++)
	{
		asm("nop");
	}
	
}

enum driveStates {dinit, high, low}dstate;
enum irStates {iinit, poll} istate;
enum brainStates {binit, okay, collision, turnright, turnleft}bstate;
enum startStates {sinit, wait, onrand, onarea, off}sstate;
enum measureStates {minit, corner, track} mstate;	
enum updateStates {uinit, send}ustate;	

unsigned short threshold;
unsigned char driveflag = 0;
unsigned char mode = 0; // 1 for rand 2 for area
#define dutycycle 5 // out of 10

#define ir_f 0
#define ir_r 1
#define ir_l 2
#define radius 5 // count for 90 degree turn

unsigned short ir_fval = 0;
unsigned short ir_rval = 0;
unsigned short ir_lval = 0;
unsigned char crash = 0;
unsigned turnflag = false;
unsigned char ready = false;
int encodercount = 0; // 88mm per 1
unsigned char l;
ISR(INT0_vect) {
	//PORTB = encodercount;
	if (mode == 1) {
		if (!turnflag) {
			++l;
			if(l > 151) {
				++encodercount;
				l = 0;			
			}
		}
	} else {
		if (ready && !turnflag) {
			++l;
			if(l > 151) {
				++encodercount;
				l = 0;
			}
		}
	}
}

void updateTick(){
	static unsigned char temp;
	switch(ustate) {
		case uinit:
			if(USART_IsSendReady(0)) {
				temp = encodercount / 100;
				USART_Send(temp, 0);
				while(!USART_HasTransmitted(0));
			}
		break;
	}

}

void measureTick() {
	if (mode != 2) {
		return;
	}
	switch(mstate) {
		 case minit:
			crash = true;;
			ready = false;
			mstate = corner;
			break;
		case corner:
			mstate = track;
			while(ir_fval < threshold) {
				PORTC = 0x05;
			}
			PORTC = 0x04;
			delay_ms(1000);
			while (ir_fval < threshold && ir_lval > threshold) {
				PORTC = 0x05;
			}
			PORTC = 0;
			break;	
			
	}
}


void driveTick() {
	if (crash == true | driveflag == false) {
		PORTC = 0;
	} else {
		PORTC = 0x05;
	}
	
}

void irTick() {
	switch (istate) {
		case iinit:
			setADC(ir_f);
			threshold = ADC + 50;
			istate = poll;
			break;
		case poll:
			setADC(ir_f);
			delay_ms(1);
			ir_fval = ADC;
		
			setADC(ir_r);
			delay_ms(1);
			ir_rval = ADC;
		
			setADC(ir_l);
			delay_ms(1);
			ir_lval = ADC;
			break;
		default:
			istate = poll;
			break;
	}
}

void brainTick() {
	if (mode != 1) {
		return;
	}
	switch (bstate) {
		case binit:
			if (mode == 1) {
				bstate = okay;
			} else {
				bstate = binit;
			}
			break;
		case okay:
			if (ir_fval > threshold) {			//ir value increases as objects get closer
				bstate = collision;
				} else {
				bstate = okay;
			}
			break;
		case collision:
			if (ir_fval > threshold) {			//ir value increases as objects get closer
				if (ir_lval < threshold && ir_rval > threshold && ir_rval > ir_lval) { // wall on right
					bstate = turnleft;
					delay_ms(500);
				} else if (ir_rval < threshold && ir_lval > threshold && ir_lval < ir_lval) { // wall on left
					bstate = turnright;
					delay_ms(500);
				} else if (ir_rval < threshold && ir_lval < threshold) { // free c
					bstate = turnright;
					delay_ms(500);
				} else {
					bstate = collision;
				}
			} else {
				bstate = okay;
			}
			break;
		case turnright:
			bstate = collision;
			break;
		case turnleft:
			bstate = collision;
			break;
		default:
			bstate = binit;
			break;
	}
	switch (bstate) {
		case binit:
			crash = true;
			break;
		case okay:
			turnflag = false;
			crash = false;
			break;
		case collision:
			crash = true;
			break;
		case turnright:
			turnflag = true;
			crash = true;
				PORTC = 0x04;		//left wheel turn
				delay_ms(1000);
			break;
		case turnleft:
			turnflag = true;
			crash = true;
				PORTC = 0x01;		//right wheel turn
				delay_ms(1000);

			break;
		default:
			break;	
		
	}

}

void startTick() {
	static unsigned char temp = 1;
	switch(sstate) {
		case sinit:
			sstate = wait;
			driveflag = false;
			mode = 0;
			break;
		case wait:
			if (USART_HasReceived(0)) {
				temp = USART_Receive(0);
				if (temp == 2) {
					sstate = onrand;
				} else if (temp == 3){
					sstate = onarea;
				} else {
					sstate = wait;
				}
			} else {
				sstate = wait;
			}
			break;
		case onrand:
			if (USART_HasReceived(0)) {
				temp = USART_Receive(0);
				if(temp == 4){
					sstate = sinit;
					driveflag = false;
					mode = 0;
				} else {
					sstate = onrand;
				}
			} else {
				sstate = onrand;
			}
			break;
		case onarea:
			if (USART_HasReceived(0)) {
				temp = USART_Receive(0);
				if(temp == 4){
					sstate = sinit;
					driveflag = false;
					mode = 0;
					l = 0;
					encodercount = 0;
				} else {
					sstate = onarea;
				}
			} else {
				sstate = onarea;
			}
			break;
	
		default:
			sstate = sinit;
			break;
	}
	switch (sstate) {
		case onrand:
			driveflag = true;
			mode = 1; 
			break;
		case onarea:
			driveflag = true;
			mode = 2;
			break;	
		case off:
			driveflag = false;
			break;
				
	}
	
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void driveTask()
{
	dstate = 0;
	for(;;)
	{
		driveTick();
		vTaskDelay(50);
	}
}

void irTask()
{
	istate = 0;
	for(;;)
	{
		irTick();
		vTaskDelay(25);
	}
}

void brainTask() {
	bstate = 0;
	for(;;)
	{
		PORTB = bstate;
		brainTick();
		vTaskDelay(50);
	}
}

void startTask() {
	sstate = 0;
	for(;;)
	{
		startTick();
		vTaskDelay(50);
	}
}

void measureTask() {
	mstate = 0;
	for(;;)
	{
		measureTick();
		vTaskDelay(50);
	}
}

void updateTask() {
	ustate = 0;
	for(;;)
	{
		updateTick();
		vTaskDelay(200);
	}
}

void drivePulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(driveTask, (signed portCHAR *)"driveTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );

}

void irPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(irTask, (signed portCHAR *)"irTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );

}

void brainPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(brainTask, (signed portCHAR *)"brainTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );

}

void startPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(startTask, (signed portCHAR *)"startTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );

}

void measurePulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(measureTask, (signed portCHAR *)"measureTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );

}

void updatePulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(updateTask, (signed portCHAR *)"updateTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );

}

int main()
{
	DDRA = 0x00; PORTA = 0xFF;
	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0x02; PORTD = 0xFD;
	
	ADC_init();
	initUSART(0);
	
	EICRA |= (1 << ISC01)|(1 <<ISC00);
	EIMSK |= (1 << INT0);
	sei();
	
	/* Replace with your application code */
	//Start Tasks
	drivePulse(0);
	irPulse(2);
	brainPulse(1);
	startPulse(0);
	measurePulse(1);
	updatePulse(3);
	//RunScheduler
	
	vTaskStartScheduler();
	
	return 0;
}

