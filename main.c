/*
        17.08.2022
        HC-SR04 Link: https://www.electronicwings.com/avr-atmega/ultrasonic-module-hc-sr04-interfacing-with-atmega1632
 */
#define F_CPU 16000000UL
#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "UART.h"

#define trigger PD3
#define echo PB0

uint64_t timer1_Overflow = 0;

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
    DDRB = 0x00;
    DDRC = 0x00;
    DDRD = 0x38;                                            // 0b0011 1000
   // PORTD = 0x04;

    PORTB = 0x01;                                           //  Pull-Up = ON
    PORTD = 0x00;

    char temperatur[] = "";
    char distance_forOut[] = "";
    uint16_t pt100_temp;
    uint16_t leitfaehig = 0;
    long count = 0;
    uint16_t distance = 0;

///***********************************************************************************************************

    TIMSK1 = (1<<TOIE1);                                    //  Timer1 Overflow Interrupt = ON
    TCCR1A = 0;

    init_usart();
    sei();

    while(1)
    {
/// PT100 Temperatur Messung
        ADMUX = 0x40;                                       //  --> PC0
        ADCSRA |= 0x07;
        ADCSRA |= (1<<ADEN);

        ADCSRA |= (1<<ADSC);
        while(ADCSRA & (1<<ADSC));
        pt100_temp = ADC;                                   //  409,6Bit @ 0°C und 490,7Bit @ 100°C ===>>   1,233Bit/°C

/// Leitfähigkeitstest
        ADMUX = 0x41;                                       //  --> PC1
        ADCSRA |= 0x07;
        ADCSRA |= (1<<ADEN);

        ADCSRA |= (1<<ADSC);
        while(ADCSRA & (1<<ADSC));
        leitfaehig = ADC;                                   //


///  Ausgabe in MyMonitor
        //itoa(pt100_temp,temperatur,10);
        //_puts(temperatur);

        PORTD ^= (1<<PD4);
        PORTD ^= (1<<PD5);
        _delay_ms(250);

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
        distance = ((((uint64_t)count/100)-375)*11)+400;    //  Messbereich: 4 bis 32cm

        itoa(leitfaehig,distance_forOut,10);
        //_delay_ms(200);
        _puts(distance_forOut);

    }

    return 0;
}
