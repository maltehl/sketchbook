#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

void setup() {
  // put your setup code here, to run once:
  DDRB = 0x0F;         // enable PB0-PB3 as outputs
  PORTB |= (1 << PB4); // enable pullup on pushbutton output and PCINT4 interrupt
  initInterrupt();
}

void loop() {
  // put your main code here, to run repeatedly:
    // initializations 

    _delay_ms(250);
    PORTB ^= (1 << PB1) | (1 << PB3);

}
static inline void initInterrupt(void)
{
  GIMSK |= (1 << PCIE);   // pin change interrupt enable
  PCMSK |= (1 << PCINT4); // pin change interrupt enabled for PCINT4
  sei();                  // enable interrupts
}

ISR(PCINT0_vect)
{
  PORTB ^= (1 << PB0) | (1 << PB2);  // toggle pins PB0 and PB2, on logical change PCINT4 pin
}
