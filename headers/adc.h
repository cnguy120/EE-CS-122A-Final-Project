/*
	Code for Reading ADC from the two joysticks
	Modified version of code given in the labs.
*/

#include <avr/io.h>

void ADC_init() {
	 ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
	 // ADEN: setting this bit enables analog-to-digital conversion.
	 // ADSC: setting this bit starts the first conversion.
	 // ADATE: setting this bit enables auto-triggering. Since we are
	 //        in Free Running Mode, a new conversion will trigger whenever
	 //        the previous conversion completes.

}

// unsigned short getInput(unsigned char select) {
// 	ADMUX = (ADMUX & 0xF8) | select; // Select the desired input (JS1 or JS2)
// 	ADCSRA |= (1<<ADSC); // Start first conversion
// 	while(ADCSRA & (1<<ADSC)); // While still converting wait
// 	return (ADC); // Return value
// }

void setADC(unsigned char pinNum) {
	ADMUX = (pinNum <= 0x07) ? pinNum : ADMUX;
	// Allow channel to stabilize
	static unsigned char i = 0;
	for ( i=0; i<32; i++ ) { asm("nop"); }
}