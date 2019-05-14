#define  __AVR_ATtinyX5__

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/power.h>

#define I2C_SLAVE_ADDRESS       0x20
#define I2C_DATA_INTERRUPT      0x80
#define I2C_BUS_COLLISION       0x08  
#define I2C_ADDRESS_STOP_MATCH  0x40 

volatile uint8_t command;
volatile uint8_t tx_buf[4];
volatile uint8_t tx_buf_index = 0;

void setup()
{
  TWSA = I2C_SLAVE_ADDRESS;
  TWSD = 0xFF;
    
  TWSCRA = (1 << TWEN)   // Two-Wire Interface Enable
         | (1 << TWSHE)  // TWI SDA Hold Time Enable
         | (1 << TWASIE) // TWI Address/Stop Interrupt Enable  
         | (1 << TWSIE)  // TWI Stop Interrupt Enable
         | (1 << TWDIE); // TWI Data Interrupt Enable  
  
  sei();  
  
}

void loop()
{
  {
    if(command != 0x00)
    {
        switch(command)    
        {
    case 0x01:
            // Test Daten 
            tx_buf[0] = 0x01;
            tx_buf[1] = 0x02;
            tx_buf[2] = 0x03;
            tx_buf[3] = 0x04;  
            tx_buf_index = 0;
            break;        
        }
      
        command = 0x00;
    }
  }
}

ISR( TWI_SLAVE_vect )
{  
  uint8_t status = TWSSRA & 0xC0;
  
  if (status & I2C_DATA_INTERRUPT) // Daten wurden vom Master empfangen oder werden angefordert
  {      
    if (TWSSRA & (1 << TWDIR)) // Master fordert Daten vom Slave an
    {
      if(tx_buf_index >= sizeof(tx_buf))
      {
        tx_buf_index=0;
      }
    
      TWSD = tx_buf[tx_buf_index];
      tx_buf_index++;
    
      TWSCRB = (uint8_t) ((1<<TWCMD1)|(1<<TWCMD0));
    }
    else // Master sendet Daten zum Slave
    {
       TWSCRB |= (uint8_t) ((1<<TWCMD1)|(1<<TWCMD0));
       command = TWSD;
    }
  }
  else if (status & TWI_ADDRESS_STOP_MATCH) 
  {    
    if (TWSSRA & TWI_BUS_COLLISION) 
    {
      TWSCRB = (uint8_t) (1<<TWCMD1);
    } 
    else 
    {    
      if (TWSSRA & (1<<TWAS)) 
      {
        // ACK 
        TWSCRB = (uint8_t) ((1<<TWCMD1)|(1<<TWCMD0));
      }  
      else 
      {
        // Stop Condition      
        TWSCRB = (uint8_t) (1<<TWCMD1);
      }
    }
  }    
}
