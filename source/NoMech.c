#include "NoMech.h"

#define IIR 2

volatile uint8_t tag;
volatile uint8_t dataready;
volatile uint16_t timerval;
volatile uint8_t ledidx;
volatile uint8_t leds[4];

extern USB_ClassInfo_CDC_Device_t NoMech_CDC_Interface;

/** Standard file stream for the CDC interface when set up, so that the virtual CDC COM port can be
 *  used like any regular character stream in the C APIs
 */
static FILE USBSerialStream;

#define XD0 0   /* Drive switches */
#define XD1 1
#define XD2 3
#define XD3 2
#define YA0 0   /* Top switches */
#define YA1 1
#define YA2 2
#define YA3 3
#define YB0 0   /* Bottom switches */
#define YB1 1
#define YB2 2
#define YB3 3

#define SW_ON   1
#define SW_OFF  0

#define sw_drive(pin,lvl)   do { \
                    switch((pin)) { \
                    case XD0: \
                        if((lvl))   PORTB |= _BV(6); \
                        else        PORTB &= ~_BV(6); \
                        break; \
                    case XD1: \
                        if((lvl))   PORTC |= _BV(6); \
                        else        PORTC &= ~_BV(6); \
                        break; \
                    case XD2: \
                        if((lvl))   PORTC |= _BV(7); \
                        else        PORTC &= ~_BV(7); \
                        break; \
                    case XD3: \
                        if((lvl))   PORTE |= _BV(2); \
                        else        PORTE &= ~_BV(2); \
                        break; \
                    } \
                } while(0)

#define sw_slope(lvl)       do { \
                    if((lvl)) { \
                        PORTB   |= _BV(0); \
                        DDRB    |= _BV(0); \
                    } else { \
                        PORTB   &= ~_BV(0); \
                        DDRB    &= ~_BV(0); \
                    } \
                } while(0)

#define sw_slope_on()   sw_slope(SW_ON)
#define sw_slope_off()  sw_slope(SW_OFF)

#define sw_bottom(pin,lvl)  do { \
                    switch((pin)) { \
                    case YB0: \
                        if((lvl))   { DDRB |= _BV(4); } \
                        else        { DDRB &= ~_BV(4); } \
                        break; \
                    case YB1: \
                        if((lvl))   { DDRF |= _BV(6); } \
                        else        { DDRF &= ~_BV(6); } \
                        break; \
                    case YB2: \
                        if((lvl))   { DDRF |= _BV(4); } \
                        else        { DDRF &= ~_BV(4); } \
                        break; \
                    case YB3:  \
                        if((lvl))   { DDRF |= _BV(0); } \
                        else        { DDRF &= ~_BV(0); } \
                        break; \
                    } \
                } while(0)

#define sw_top(pin,lvl)     do { \
                    switch((pin)) { \
                    case YA0: \
                        if((lvl))   {  DDRB |= _BV(5); } \
                        else        {  DDRB &= ~_BV(5); } \
                        break; \
                    case YA1: \
                        if((lvl))   {  DDRF |= _BV(7); } \
                        else        {  DDRF &= ~_BV(7); } \
                        break; \
                    case YA2: \
                        if((lvl))   {  DDRF |= _BV(5); } \
                        else        {  DDRF &= ~_BV(5); } \
                        break; \
                    case YA3: \
                        if((lvl))   {  DDRF |= _BV(1); } \
                        else        {  DDRF &= ~_BV(1); } \
                        break; \
                    } \
                } while(0)

#define sc_on(pin)      do { \
                    switch((pin)) { \
                    case YB0: ADMUX = 0b00000011; ADCSRB |= _BV(MUX5); break; /* ADC11 */ \
                    case YB1: ADMUX = 0b00000110; ADCSRB &= ~_BV(MUX5); break; /* ADC6 */ \
                    case YB2: ADMUX = 0b00000100; ADCSRB &= ~_BV(MUX5); break; /* ADC4 */ \
                    case YB3: ADMUX = 0b00000000; ADCSRB &= ~_BV(MUX5); break; /* ADC0 */ \
                    } \
                    ACSR = _BV(ACIE) | _BV(ACI) | _BV(ACIC) | _BV(ACIS1) ;\
                } while(0)

#define sc_off()        do { \
                    ACSR = 0; \
                } while(0)

#define tmr_reset()     do { \
                    TCNT1H = 0; \
                    TCNT1L = 0; \
                } while(0)

typedef void (*pf_t)(uint8_t);

#define pumpfunc(x,y)   void pump_##y##_##x(uint8_t n) {\
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
                fprintf(&USBSerialStream, "***********************\r\n");
                fprintf(&USBSerialStream, "* %4u %4u %4u %4u *\r\n", res[3][3], res[2][3], res[1][3], res[0][3]);
                fprintf(&USBSerialStream, "* %4u %4u %4u %4u *\r\n", res[3][2], res[2][2], res[1][2], res[0][2]);
                fprintf(&USBSerialStream, "* %4u %4u %4u %4u *\r\n", res[3][1], res[2][1], res[1][1], res[0][1]);
                fprintf(&USBSerialStream, "* %4u %4u %4u %4u *\r\n", res[3][0], res[2][0], res[1][0], res[0][0]);
                fprintf(&USBSerialStream, "***********************\r\n\r\n");
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
    DDRB       |=  _BV(PB2);    // MOSI out

    TCCR1B = _BV(ICES1) | _BV(WGM12) | _BV(CS10);   /* Timer 1 free running */
    OCR1A = 0xffff;
    TCCR3B = _BV(WGM32) | _BV(CS30);    /* Timer 3 free running */
    OCR3A = 0x7fff;
    TIMSK3 = _BV(OCIE3A); 

    USB_Init();
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
