#include <TinyWireM.h>
#include <USI_TWI_Master.h>

#define I2C_SLAVE_ADDRESS 0x4 // Address of the slave

#include <avr/sleep.h>
#include <avr/interrupt.h>

const int rainPin = 3; // which pin is rain sensor connected to
const int windPin = 4; // which pin is wind sensor connected to
const int statusLED = 1; //which pin is status LED connected to


volatile int raincount = 0;
volatile int windcount = 0;
volatile int t=0;
byte lowWind;
byte highWind;
//volatile String requestType = "";
volatile int response;
//TineWireM TinyWireS;
 
void setup()
{
   TinyWireM.begin(I2C_SLAVE_ADDRESS); // join i2c network
   TinyWireM.onReceive(receiveEvent);
   TinyWireM.onRequest(requestEvent);

   pinMode(rainPin, INPUT);
   digitalWrite(rainPin, HIGH);
   pinMode(windPin, INPUT);
   digitalWrite(windPin, HIGH);
   pinMode(statusLED, OUTPUT);
           
   } // setup


void sleep() {

   GIMSK |= _BV(PCIE);                     // Enable Pin Change Interrupts
   PCMSK |= _BV(PCINT3);                   // Use PB3 as interrupt pin
   PCMSK |= _BV(PCINT4);                   // Use PB4 as interrupt pin

   ADCSRA &= ~_BV(ADEN);                   // ADC off
   //WDTCR &= ~_BV(WDIE);                    // DISABLE WATCHDOG
   set_sleep_mode(SLEEP_MODE_PWR_DOWN);    // replaces above statement

   sleep_enable();                         // Sets the Sleep Enable bit in the MCUCR Register (SE BIT)
   sei();                                  // Enable interrupts
   sleep_cpu();                            // sleep

   cli();                                  // Disable interrupts
   PCMSK &= ~_BV(PCINT3);                  // Turn off PB3 as interrupt pin
   PCMSK &= ~_BV(PCINT4);                  // Turn off PB4 as interrupt pin
   sleep_disable();                        // Clear SE bit
   ADCSRA |= _BV(ADEN);                    // ADC on
   //WDTCR |= _BV(WDIE);                    // Watchdog on

   sei();                                  // Enable interrupts
   } // sleep


ISR(PCINT0_vect) {
 // DO NOTHING HERE, CATCH EVERYTHING IN LOOP // Count rain gauge bucket tips as they occur
}

void loop()
{
 if (digitalRead (rainPin) == LOW) {
   raincount ++;
   }
 
 if (digitalRead (windPin) == HIGH) {
   windcount ++;
 }

     
 // This needs to be here
 TinyWireM_stop_check();

 // Go back to sleep
 sleep();
 tws_delay(1);

}

// Gets called when the ATtiny receives an i2c request
void requestEvent()
{
   //send raincount (max rain = 3" per minute)
   TinyWireM.send(raincount);

   //int windcount = 12345;
 
   byte lowWind = (byte)(windcount & 0xff);
   byte highWind = (byte)((windcount >> 8) & 0xff);
   //t=0;
   TinyWireM.send(lowWind);
   TinyWireM.send(highWind);  
   //tws_delay(50);
}


// Gets called when the ATtiny receives an i2c write slave request
void receiveEvent(int howMany)
{
 for( int i = 0; i < howMany; i++)
 {
   while(TinyWireM.available())
   {
     response = TinyWireM.receive();
     if (response == 1) {
       raincount = 0;
       //requestType = "resetrain";
     } else if (response == 2) {
       windcount = 0;  
       //requestType = "wind";
     } else {
         // DO NOTHING
     }
   }
   //tws_delay(100);
 }
}
