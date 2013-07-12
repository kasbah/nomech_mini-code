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

//#define TOP         PB4
//#define DDR_TOP     DDRB
//#define PORT_TOP    PORTB
//
//#define BOTTOM      PB5
//#define DDR_BOTTOM  DDRB
//#define PORT_BOTTOM PORTB

#define TOP         PF1
#define DDR_TOP     DDRF
#define PORT_TOP    PORTF

#define BOTTOM      PF0
#define DDR_BOTTOM  DDRF
#define PORT_BOTTOM PORTF

#define SLOPE       PB7
#define DDR_SLOPE   DDRB
#define PORT_SLOPE  PORTB

#define DRIVE       PB6
#define DDR_DRIVE   DDRB
#define PORT_DRIVE  PORTB



#include "NoMech.h"
#include <util/delay.h>

volatile bool done = false;
volatile uint16_t measured = 0;
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


/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */

int main(void)
{
    SetupHardware();

    /* Create a regular character stream for the interface so that it can be used with the stdio.h functions */
    CDC_Device_CreateStream(&NoMech_CDC_Interface, &USBSerialStream);

    GlobalInterruptEnable();

    for (;;)
    {


        PORT_BOTTOM &= ~(1 << BOTTOM);
        PORT_TOP    &= ~(1 << TOP);
        PORT_SLOPE  &= ~(1 << SLOPE);
        DDR_SLOPE   &= ~(1 << SLOPE);

        for (int j = 0; j < 200; j++)
        {
            pump();
        }

        ADMUX       &= 0b00000;

        _delay_us(1);

        PORT_SLOPE  |= (1 << SLOPE);
        DDR_SLOPE   |= (1 << SLOPE);

        measured = 0;
        do {
            done = !((ACSR & (1 <<ACO)) >> ACO);
            measured++; 
        } while (!done);

        fprintf(&USBSerialStream, "measured: %i\r\n", measured);

        PORTB       &= ~(1 << PB0);

        DDR_SLOPE   &= ~(1 << SLOPE);
        PORT_SLOPE  &= ~(1 << SLOPE);

        DDR_TOP     |=  (1 << TOP);
        DDR_BOTTOM  |=  (1 << BOTTOM);

        PORT_TOP    &= ~(1 << TOP);
        PORT_BOTTOM &= ~(1 << BOTTOM);

        CDC_Device_USBTask(&NoMech_CDC_Interface);
        USB_USBTask();
    }
}

void pump(void)
{

    DDR_TOP    &= ~(1 << TOP);
    DDR_BOTTOM |=  (1 << BOTTOM);

    PORT_DRIVE |=  (1 << DRIVE);

    DDR_BOTTOM &= ~(1 << BOTTOM);
    DDR_TOP    |=  (1 << TOP);

    PORT_DRIVE &= ~(1 << DRIVE);


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
    DIDR1      |=  (1 << AIN0D);

    //set the drive pin to Hi-Z 
    DDR_DRIVE  |=  (1 << DRIVE);
    PORT_DRIVE &= ~(1 << DRIVE);

    //enable the analog MUX for the comparator
    ADCSRB     |=  (1 << ACME);

    DDRB       |=  (1 << PB0);

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

//ISR(TIMER1_CAPT_vect)
//{
//    measured = ICR1;
//
//  ACSR &= ~(1 << ACIC);   // disable AC capture input
//    TIMSK1 &= ~(1 << ICIE1);
//  TCCR1B = 0b00000000;
//
//    done = true;
//}
