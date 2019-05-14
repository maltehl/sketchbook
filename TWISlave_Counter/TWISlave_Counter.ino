/* Sleep Demo Serial
 * -----------------
 * Example code to demonstrate the sleep functions in a Arduino. Arduino will wake up
 * when new data is received in the serial port USART
 * Based on Sleep Demo Serial from http://www.arduino.cc/playground/Learning/ArduinoSleepCode 

 "Aufwach-Taster" an Pin x gegen Masse
 
 Arduino Mega 2560
*/   
   

#include <avr/sleep.h>
#include <TinyWireS_Custom.h>
#include <EEPROM.h>

#define I2CADDRESS 0x26

int led0 = 1;            // LED connected to digital pin 30


int wake0 = 4;            // active LOW, ground this pin momentary to wake up
int wake1 = 5;            // active LOW, ground this pin momentary to wake up



struct CounterStruct
{
  unsigned int rainCount;                        // (B0-1) 4 byte ID, wipes eeprom if missing
  unsigned int windCount;                    // (B2-3) Version, change if struct changes, wipes eeprom
} 
Counter;

volatile byte I2CBuffer[16];                // Buffer to store incoming I2C message
volatile byte I2CReceived=0;                // flag, will be set if I2C message received
byte CounterPointer = 0;                   // pointer to specifig location in the settings struct


int ledState0 = LOW;

void wakeUpNow0()        // here the interrupt is handled after wakeup
{
  // execute code here after wake-up before returning to the loop() function
  // timers and code using timers (serial.print and more...) will not work here.
  // digitalWrite(interruptPin, HIGH);  // LED an Pin ?? ein
  Counter.rainCount ++;
}

void wakeUpNow1() 
{ 
  Counter.windCount ++;
  
}       // here the interrupt is handled after wakeup



void sleepNow()
{
    /* Now is the time to set the sleep mode. In the Atmega8 datasheet
     * http://www.atmel.com/dyn/resources/prod_documents/doc2486.pdf on page 35
     * there is a list of sleep modes which explains which clocks and 
     * wake up sources are available in which sleep modus.
     *
     * In the avr/sleep.h file, the call names of these sleep modus are to be found:
     *
     * The 5 different modes are:
     *     SLEEP_MODE_IDLE         -the least power savings 
     *     SLEEP_MODE_ADC
     *     SLEEP_MODE_PWR_SAVE
     *     SLEEP_MODE_STANDBY
     *     SLEEP_MODE_PWR_DOWN     -the most power savings
     *
     *  the power reduction management <avr/power.h>  is described in 
     *  http://www.nongnu.org/avr-libc/user-manual/group__avr__power.html
     */  
       
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);   // sleep mode is set here

  sleep_enable();          // enables the sleep bit in the mcucr register
                           // so sleep is possible. just a safety pin 
  
    /* Now it is time to enable an interrupt. We do it here so an
     * accidentally pushed interrupt button doesn't interrupt
     * our running program. if you want to be able to run
     * interrupt code besides the sleep function, place it in
     * setup() for example.
     *
     * In the function call attachInterrupt(A, B, C)
     * A   can be either 0 or 1 for interrupts on pin 2 or 3.  
     *
     * B   Name of a function you want to execute at interrupt for A.
     *
     * C   Trigger mode of the interrupt pin. can be:
     *             LOW        a low level triggers
     *             CHANGE     a change in level triggers
     *             RISING     a rising edge of a level triggers
     *             FALLING    a falling edge of a level triggers
     *
     * In all but the IDLE sleep modes only LOW can be used.
     */
 
  attachInterrupt(0, wakeUpNow0, LOW); // use interrupt 0 (pin 2) and run function
                                     // wakeUpNow when pin 2 gets LOW 

  attachInterrupt(1, wakeUpNow1, LOW); // use interrupt 1 (pin 3) and run function
                                       // wakeUpNow when pin 3 gets LOW                                    
      
  sleep_mode();            // here the device is actually put to sleep!!
 
                           // THE PROGRAM CONTINUES FROM HERE AFTER WAKING UP
  sleep_disable();         // first thing after waking from sleep:
                           // disable sleep...
  
  detachInterrupt(0);      // disables interrupt 0 on pin 2 so the
                           // wakeUpNow code will not be executed
                           // during normal running time.
  

  
  detachInterrupt(1);      // disables interrupt 1 on pin 3 so the
                           // wakeUpNow code will not be executed
                           // during normal running time.                                   
                             

  
}


void setup()
{

  TinyWireS.begin(I2CADDRESS);
  TinyWireS.onRequest(requestEvent);
  TinyWireS.onReceive(receiveEvent);
    
  pinMode(led0, OUTPUT);       // LED connected to digital pin 1

 
  pinMode(wake0, INPUT);         // active LOW, ground this pin momentary to wake up
  digitalWrite(wake0, HIGH);     // Pullup aktiv
  
  pinMode(wake1, INPUT);         // active LOW, ground this pin momentary to wake up
  digitalWrite(wake1, HIGH);     // Pullup aktiv

  Counter.rainCount = 0;
  Counter.windCount = 0;
       
  attachInterrupt(0, wakeUpNow0, LOW);   // use interrupt 0 (pin 2) and run function wakeUpNow when pin 2 gets LOW 
  attachInterrupt(1, wakeUpNow1, LOW);   // use interrupt 1 (pin 3) and run function wakeUpNow when pin 2 gets LOW    
                         
                            
}


void loop()
{
   //INT 0  
   if ( digitalRead(wake0) == LOW ) {
      if ( ledState0 == LOW ) {
         ledState0 = HIGH;
       } else {
         ledState0 = LOW;
       }
     digitalWrite(led0, ledState0); 
   }
   
   // INT 1
   if ( digitalRead(wake1) == LOW ) {
     if ( ledState0 == LOW ) {
         ledState0 = HIGH;
       } else {
         ledState0 = LOW;
       }
     digitalWrite(led0, ledState0); 
   }
  
 sleepNow();                   // sleep function called here
}


/********************************************************************************************\
 * Reply to I2C read requests 
 \*********************************************************************************************/
void requestEvent()
{
  byte *pointerToByteToRead = (byte*)&Counter + CounterPointer;
  TinyWireS.send(*pointerToByteToRead);
}

/********************************************************************************************\
 * Receive and store incoming I2C writes 
 \*********************************************************************************************/
void receiveEvent(uint8_t howMany)
{

  if (howMany > 16)
    return;

  I2CReceived = howMany;

  for(byte x=0; x < howMany; x++)
    I2CBuffer[x] = TinyWireS.receive();
}



