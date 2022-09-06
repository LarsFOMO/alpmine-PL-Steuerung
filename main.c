///
///                 19.08.2022
///        alpmine PL Sicherheitssteuerung
///                 Lars Kager
///

/// INFO
/// Wenn eine Lampe leuchtet (PD5) gibt es ein Problem, wenn beide Lampen leuchten befindet sich das Programm im Startvorgang.

#define F_CPU 16000000UL
#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "UART.h"

#define trigger PD3
#define echo PB0
#define ssr1 PC2
#define ssr2 PC3

void ssr_off(void);
void delay_1min(uint8_t anzahl);

uint64_t timer1_Overflow = 0;
uint8_t i = 0;

ISR(TIMER1_OVF_vect)
{
    uint8_t sreg = SREG;
    cli();

    timer1_Overflow++;

    SREG = sreg;
}

int main(void)
{

///***********************************************************************************************************
/// INIT SETUP
    DDRB = 0x00;                                            //  PB0 = Echo
    DDRC = 0x0C;                                            //  PC0 = PT100; PC1 = Leitfähigkeit; PC2 = SSR1; PC3 = SSR2
    DDRD = 0x38;                                            //  PD3 = Trigger; PD4 = LED1; PD5 = LED2

    PORTB = 0x01;                                           //  PB0: Pull-Up = ON
    PORTD = 0x00;

    uint16_t pt100_temp;
    uint16_t leitfaehig = 0;
    long count = 0;
    uint16_t distance = 0;
    uint8_t ok = 0;
    //char test1[] = "";
    //char test2[] = "";

    TIMSK1 = (1<<TOIE1);                                    //  Timer1 Overflow Interrupt = ON
    TCCR1A = 0;

    init_usart();
    sei();

/// Startpause [1 min]
    PORTD |= (1<<PD4);
    delay_1min(3);
    PORTD &= ~(1<<PD4);

///***********************************************************************************************************

    while(1)
    {
        ok = 0;

/// Leitfähigkeitstest
        ADMUX = 0x41;                                       //  --> PC1
        ADCSRA |= 0x07;
        ADCSRA |= (1<<ADEN);

        ADCSRA |= (1<<ADSC);
        while(ADCSRA & (1<<ADSC));
        leitfaehig = ADC;                                   //  Leitfähigkeit == 1023 ===>> Achtung Wasser!

        //leitfaehig = 0;
        if(leitfaehig <= 100)
        {
            ok++;
        }
        else
            ssr_off();

/// PT100 Temperatur Messung
        ADMUX = 0x40;                                       //  --> PC0
        ADCSRA |= 0x07;
        ADCSRA |= (1<<ADEN);

        ADCSRA |= (1<<ADSC);
        while(ADCSRA & (1<<ADSC));
        pt100_temp = ADC;                                   //  409,6Bit @ 0°C und 490,7Bit @ 100°C ===>>   0,811°C/Bit

        //pt100_temp = 400;
        if(pt100_temp <= 463)                               // Temperatur unter 60°C
        {
            ok++;
        }
        else
            ssr_off();

///  10us Trigger Impuls
        PORTD |= (1<<trigger);
        _delay_us(10);
        PORTD &= ~(1<<trigger);
///  Timer Setup
        TCNT1 = 0;
        TCCR1B = 0x41;                                      //  aktiv bei steigende Flanke, kein Prescaler
        TIFR1 = (1<<ICF1);                                  //  clear input Capture Flag
        TIFR1 = (1<<TOV1);                                  //  clear Timer Overflow Flag
///  Distanz berechnen (width of Echo by ICF)
        while((TIFR1 && (1<<ICF1)) == 0);                   //  Wartet auf steigende Flanke
        TCNT1 = 0;
        TCCR1B = 0x01;                                      //  aktiv bei fallende Flanke, kein PS
        TIFR1 = (1<<ICF1);                                  //  clear ICF1
        TIFR1 = (1<<TOV1);                                  //  clear TOV1
        timer1_Overflow = 0;
///  Berechnung Distanz
        while((TIFR1 && (1<<ICF1)) == 0);                   //  Wartet auf fallende Flanke
        count = ICR1 + (65535 * timer1_Overflow);
        distance = ((((uint64_t)count/100)-375)*11)+400;    //  Messbereich: 4 bis 32cm ==>>    400 bis 3200

        //distance = 500;
        if((distance < 750) && (distance > 450))
        {
            ok++;
        }
        else
            ssr_off();

        if(ok == 3)
        {
            PORTC |= (1<<ssr1);
            PORTC |= (1<<ssr2);
            PORTD |= (1<<PD4);
            PORTD |= (1<<PD5);
        }

/// Alarm blinken
//        else if(ok < 3)
//        {
        PORTD ^= (1<<PD4);
        PORTD ^= (1<<PD5);
//        }

/// Delay
        _delay_ms(200);
        //itoa(leitfaehig, test2, 10);
        //_puts(test2);
    }

    return 0;
}

///***********************************************************************************************************
/// FUNCTIONS

void ssr_off(void)
{
    PORTC &= ~(1<<ssr1);
    PORTC &= ~(1<<ssr2);
    delay_1min(6);                                          //  Delay = 2 min.
}

void delay_1min(uint8_t anzahl)
{
    PORTD |= (1<<PD5);

    for(i = 0; i < anzahl;i++)                              //  Anzahl = 3 ==>> 1 min.
    {
        _delay_ms(1000000);
        _delay_ms(1000000);
        _delay_ms(1000000);
        _delay_ms(1000000);
        _delay_ms(1000000);
    }

    PORTD &= ~(1<<PD5);
}
///***********************************************************************************************************
