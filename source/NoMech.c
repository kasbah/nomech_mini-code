/*
             LUFA Library
     Copyright (C) Dean Camera, 2013.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2013  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Main source file for the NoMech demo. This file contains the main tasks of
 *  the demo and is responsible for the initial application hardware configuration.
 */

#define TOP         PB5
#define DDR_TOP     DDRB
#define PORT_TOP    PORTB

#define BOTTOM      PB4
#define DDR_BOTTOM  DDRB
#define PORT_BOTTOM PORTB

#define SLOPE       PB0
#define DDR_SLOPE   DDRB
#define PORT_SLOPE  PORTB

#define DRIVE       PB6
#define DDR_DRIVE   DDRB
#define PORT_DRIVE  PORTB

#define MAX_PUMPS 100 
#define BUF_SIZE 64 

#include "NoMech.h"
#include <util/delay.h>

volatile uint16_t measured[BUF_SIZE];
//volatile uint16_t measured;
volatile uint8_t read_index  = 0;
volatile uint8_t write_index = 0;
volatile int number_of_pumps = 0;
volatile uint8_t tag;
volatile uint8_t dataready;
volatile uint16_t timerval;
volatile uint8_t ledidx;
volatile uint8_t leds[4];


/** LUFA CDC Class driver interface configuration and state information. This structure is
 *  passed to all CDC Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_CDC_Device_t NoMech_CDC_Interface =
    {
        .Config =
            {
                .ControlInterfaceNumber   = 0,
                .DataINEndpoint           =
                    {
                        .Address          = CDC_TX_EPADDR,
                        .Size             = CDC_TXRX_EPSIZE,
                        .Banks            = 1,
                    },
                .DataOUTEndpoint =
                    {
                        .Address          = CDC_RX_EPADDR,
                        .Size             = CDC_TXRX_EPSIZE,
                        .Banks            = 1,
                    },
                .NotificationEndpoint =
                    {
                        .Address          = CDC_NOTIFICATION_EPADDR,
                        .Size             = CDC_NOTIFICATION_EPSIZE,
                        .Banks            = 1,
                    },
            },
    };

/** Standard file stream for the CDC interface when set up, so that the virtual CDC COM port can be
 *  used like any regular character stream in the C APIs
 */
static FILE USBSerialStream;

#define XD0	0	/* Drive switches */
#define XD1	1
#define XD2	3
#define XD3	2
#define YA0	0	/* Top switches */
#define YA1	1
#define YA2	2
#define YA3	3
#define YB0	0	/* Bottom switches */
#define YB1	1
#define YB2	2
#define YB3	3

#define	SW_ON	1
#define SW_OFF	0

#define sw_drive(pin,lvl)	do { \
					switch((pin)) { \
					case XD0: \
						if((lvl))	PORTB |= _BV(6); \
						else		PORTB &= ~_BV(6); \
						break; \
					case XD1: \
						if((lvl))	PORTC |= _BV(6); \
						else		PORTC &= ~_BV(6); \
						break; \
					case XD2: \
						if((lvl))	PORTC |= _BV(7); \
						else		PORTC &= ~_BV(7); \
						break; \
					case XD3: \
						if((lvl))	PORTE |= _BV(2); \
						else		PORTE &= ~_BV(2); \
						break; \
					} \
				} while(0)

#define sw_slope(lvl)		do { \
					if((lvl)) { \
						PORTB	|= _BV(0); \
						DDRB	|= _BV(0); \
					} else { \
						PORTB	&= ~_BV(0); \
						DDRB	&= ~_BV(0); \
					} \
				} while(0)

#define sw_slope_on()	sw_slope(SW_ON)
#define sw_slope_off()	sw_slope(SW_OFF)

#define sw_bottom(pin,lvl)	do { \
					switch((pin)) { \
					case YB0: \
						if((lvl))	{  DDRB |= _BV(4); } \
						else		{   DDRB &= ~_BV(4); } \
						break; \
					case YB1: \
						if((lvl))	{  DDRF |= _BV(6); } \
						else		{   DDRF &= ~_BV(6); } \
						break; \
					case YB2: \
						if((lvl))	{  DDRF |= _BV(4); } \
						else		{   DDRF &= ~_BV(4); } \
						break; \
					case YB3:  \
						if((lvl))	{  DDRF |= _BV(0); } \
						else		{   DDRF &= ~_BV(0); } \
						break; \
					} \
				} while(0)

#define sw_top(pin,lvl)		do { \
					switch((pin)) { \
					case YA0: \
						if((lvl))	{  DDRB |= _BV(5); } \
						else		{  DDRB &= ~_BV(5); } \
						break; \
					case YA1: \
						if((lvl))	{  DDRF |= _BV(7); } \
						else		{  DDRF &= ~_BV(7); } \
						break; \
					case YA2: \
						if((lvl))	{  DDRF |= _BV(5); } \
						else		{  DDRF &= ~_BV(5); } \
						break; \
					case YA3: \
						if((lvl))	{  DDRF |= _BV(1); } \
						else		{  DDRF &= ~_BV(1); } \
						break; \
					} \
				} while(0)

#define sc_on(pin)		do { \
					switch((pin)) { \
					case YB0: ADMUX = 0b00000011; ADCSRB |= _BV(MUX5); break; /* ADC11 */ \
					case YB1: ADMUX = 0b00000110; ADCSRB &= ~_BV(MUX5); break; /* ADC6 */ \
					case YB2: ADMUX = 0b00000100; ADCSRB &= ~_BV(MUX5); break; /* ADC4 */ \
					case YB3: ADMUX = 0b00000000; ADCSRB &= ~_BV(MUX5); break; /* ADC0 */ \
					} \
					ACSR = _BV(ACIE) | _BV(ACI) | _BV(ACIC) | _BV(ACIS1) ;\
				} while(0)

#define sc_off()		do { \
					ACSR = 0; \
				} while(0)

#define tmr_reset()		do { \
					TCNT1H = 0; \
					TCNT1L = 0; \
				} while(0)

typedef void (*pf_t)(uint8_t);

#define pumpfunc(x,y)	void pump_##y##_##x(uint8_t n) {\
		uint8_t i; \
	PORTB |= _BV(2); \
		sw_drive(x, SW_OFF); \
		sw_bottom(y, SW_ON); \
		sw_top(y, SW_ON); \
		__asm("nop"); \
		__asm("nop"); \
		__asm("nop"); \
		__asm("nop"); \
		__asm("nop"); \
		__asm("nop"); \
		__asm("nop"); \
		__asm("nop"); \
		__asm("nop"); \
		__asm("nop"); \
 \
		for(i = 0; i < n; i++) { \
			sw_top(y, SW_OFF); \
			sw_bottom(y, SW_ON); \
			sw_drive(x, SW_ON); \
			sw_bottom(y, SW_OFF); \
			sw_top(y, SW_ON); \
			sw_drive(x, SW_OFF); \
		} \
	PORTB &= ~_BV(2); \
	}

pumpfunc(0,0)
pumpfunc(0,1)
pumpfunc(0,2)
pumpfunc(0,3)
pumpfunc(1,0)
pumpfunc(1,1)
pumpfunc(1,2)
pumpfunc(1,3)
pumpfunc(2,0)
pumpfunc(2,1)
pumpfunc(2,2)
pumpfunc(2,3)
pumpfunc(3,0)
pumpfunc(3,1)
pumpfunc(3,2)
pumpfunc(3,3)

const pf_t pumpers[16] = {
	pump_0_0, pump_0_1, pump_0_2, pump_0_3,
	pump_1_0, pump_1_1, pump_1_2, pump_1_3,
	pump_2_0, pump_2_1, pump_2_2, pump_2_3,
	pump_3_0, pump_3_1, pump_3_2, pump_3_3,
};

/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */

int main(void)
{
	SetupHardware();

	/* Create a regular character stream for the interface so that it can be used with the stdio.h functions */
	CDC_Device_CreateStream(&NoMech_CDC_Interface, &USBSerialStream);

	GlobalInterruptEnable();
	uint16_t ch;
	uint8_t x = 0;
	uint8_t y = 0;
	static uint16_t avg[4][4];
	static uint16_t res[4][4];
	uint8_t p = 0;

	for(;;) {
		while(!tag) {
			/* Must throw away unused bytes from the host, or it will lock up while waiting for the device */
			if(-1 != (ch = CDC_Device_ReceiveByte(&NoMech_CDC_Interface)))
				CDC_Device_SendByte(&NoMech_CDC_Interface, ch);
		}
		tag = 0;

		sc_off();
		sw_slope_off();
		pumpers[(y << 2) + x](200);
		tmr_reset();
		sc_on(y);
		sw_slope_on();
		while(!dataready && !tag) {
			if(-1 != (ch = CDC_Device_ReceiveByte(&NoMech_CDC_Interface)))
				CDC_Device_SendByte(&NoMech_CDC_Interface, ch);
			CDC_Device_USBTask(&NoMech_CDC_Interface);
			USB_USBTask();
		}
		dataready = 0;
#define IIR	2
		res[y][x] = timerval + 2048 - (avg[y][x] >> IIR);
		avg[y][x] = ((avg[y][x] * ((1<<IIR)-1)) >> IIR) + timerval;



		CDC_Device_USBTask(&NoMech_CDC_Interface);
		USB_USBTask();
		if(++x > 3) {
			x = 0;
			p++;
			p &= 0x03;
			if(++y > 3) {
				y = 0;
				fprintf(&USBSerialStream, "%4u %4u %4u %4u %4u %4u %4u %4u %4u %4u %4u %4u %4u %4u %4u %4u\r\n",
						res[0][0], res[0][1], res[0][2], res[0][3], res[1][0], res[1][1], res[1][2], res[1][3],
						res[2][0], res[2][1], res[2][2], res[2][3], res[3][0], res[3][1], res[3][2], res[3][3]);
			}
		}
		leds[x] = 1 << p;
	}
}


/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR      &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	/* Hardware Initialization */

	//disable logic on AIN0 pin
	DIDR1      |=  _BV(AIN0D);

	PORTB = 0b00001110;
	PORTC = 0;
	PORTD = 0;
	PORTE = 0;
	PORTF = 0;

	DDRB = 0b11000001;
	DDRC = 0b11000000;
	DDRD = 0b11111011;
	DDRE = 0b00000100;
	DDRF = 0;

	DIDR0 = 0b11110011;
	DIDR2 = 0b00011000;

	//enable the analog MUX for the comparator
	ADCSRB     |=  _BV(ACME);

	//extra pin used for debugging
	DDRB       |=  _BV(PB2);	// MOSI out

	TCCR1B = _BV(ICES1) | _BV(WGM12) | _BV(CS10);	/* Timer 1 free running */
	OCR1A = 0xffff;
	TCCR3B = _BV(WGM32) | _BV(CS30);	/* Timer 3 free running */
	OCR3A = 0x7fff;
	TIMSK3 = _BV(OCIE3A); 

	USB_Init();
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
    bool ConfigSuccess = true;

    ConfigSuccess &= CDC_Device_ConfigureEndpoints(&NoMech_CDC_Interface);

}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
    CDC_Device_ProcessControlRequest(&NoMech_CDC_Interface);
}

ISR(ANALOG_COMP_vect)
{
PORTB |= _BV(2);
	//sw_slope_off();
	sc_off();
	dataready = 1;
	//timerval = ICR1;
	timerval = TCNT1L;
	timerval |= TCNT1H << 8;
PORTB &= ~_BV(2);
}

ISR(TIMER3_COMPA_vect)
{
	PORTD |= 0b00000011;
	PORTD &= 0b00001111;
	if(!(ledidx & 1))
		PORTB |= _BV(7);
	else
		PORTB &= ~_BV(7);
	PORTD |= (leds[ledidx] << 4);
	PORTD &= (ledidx & 2) ? ~0x01 : ~0x02;
	ledidx++;
	ledidx &= 0x3;

	tag = 1;
}
