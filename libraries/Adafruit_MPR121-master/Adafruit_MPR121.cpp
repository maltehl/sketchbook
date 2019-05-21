/*************************************************** 
  This is a library for the MPR121 I2C 12-chan Capacitive Sensor

  Designed specifically to work with the MPR121 sensor from Adafruit
  ----> https://www.adafruit.com/products/1982

  These sensors use I2C to communicate, 2+ pins are required to  
  interface
  Adafruit invests time and resources providing this open source code, 
  please support Adafruit and open-source hardware by purchasing 
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution
 ****************************************************/

#include "Adafruit_MPR121.h"

Adafruit_MPR121::Adafruit_MPR121() {
}

boolean Adafruit_MPR121::begin(uint8_t i2caddr, uint8_t  SDA, uint8_t  SCL) {
    Wire.begin(SDA,SCL);

    
  _i2caddr = i2caddr;

  // soft reset
  writeRegister(MPR121_SOFTRESET, 0x63);
  delay(1);
  for (uint8_t i=0; i<0x7F; i++) {
  //  Serial.print("$"); Serial.print(i, HEX); 
  //  Serial.print(": 0x"); Serial.println(readRegister8(i));
  }
  

  writeRegister(MPR121_ECR, 0x0);

  uint8_t c = readRegister8(MPR121_CONFIG2);
  
  if (c != 0x24) return false;


  setThreshholds(12, 6);
  writeRegister(MPR121_MHDR, 0x01);
  writeRegister(MPR121_NHDR, 0x01);
  writeRegister(MPR121_NCLR, 0x0E);
  writeRegister(MPR121_FDLR, 0x00);

  writeRegister(MPR121_MHDF, 0x01);
  writeRegister(MPR121_NHDF, 0x05);
  writeRegister(MPR121_NCLF, 0x01);
  writeRegister(MPR121_FDLF, 0x00);

  writeRegister(MPR121_NHDT, 0x00);
  writeRegister(MPR121_NCLT, 0x00);
  writeRegister(MPR121_FDLT, 0x00);

  writeRegister(MPR121_DEBOUNCE, 0);
  writeRegister(MPR121_CONFIG1, 0x01); // default, 1uA charge current
  writeRegister(MPR121_CONFIG2, 0x20); // 0.5uS encoding, 1ms period
 
 // writeRegister(MPR121_AUTOCONFIG0, 0x7F); // only C0 // 10 Samples; 8 Retry //01|11|11|11 - 10Samples
 // writeRegister(MPR121_AUTOCONFIG0, 0x8F); // Default // 18 Samples; no Retry
  writeRegister(MPR121_AUTOCONFIG0, 0x7F); // Default // 18 Samples; no Retry //01|11|11|11 -10 Samples -8 Retry on Failure -Baseline change -Enable AutoConfig and AutoReConfig
 //11 ->AFES 34 samples
 //11 -> RETRY 8 times
 //11 -> BVA  Baseline is set to the AUTO-CONFIG baselinevalue
 //1  -> ARE Enable Autoreconfig
 //1  -> ACE Enable Autoconfig
 

  writeRegister(MPR121_AUTOCONFIG1, 0x00); // only C0
//  0|XXXX|000 - Autoset and Not Interrupts 

  writeRegister(MPR121_UPLIMIT, 202);
  writeRegister(MPR121_TARGETLIMIT, 182); // should be ~400 (100 shifted)
  writeRegister(MPR121_LOWLIMIT, 131);
  // enable all electrodes
  writeRegister(MPR121_ECR, 0x8F);  // start with first 5 bits of baseline tracking No proximity detection
//  writeRegister(MPR121_ECR, 0xC1);  // start with first 5 bits of baseline tracking All Probes enable
//10 = CL -> BaselineTracking
//00= ELEPROX -> Proximity detection diable
//11xx = Run Mode ELE0-11 Measurement enable
  

  return true;
  return true;
}

void Adafruit_MPR121::setThreshholds(uint8_t touch, uint8_t release) {

  setThresholds(touch, release);
  }

void Adafruit_MPR121::setThresholds(uint8_t touch, uint8_t release) {
  for (uint8_t i=0; i<12; i++) {
    writeRegister(MPR121_TOUCHTH_0 + 2*i, touch);
    writeRegister(MPR121_RELEASETH_0 + 2*i, release);
  }
}

uint16_t  Adafruit_MPR121::filteredData(uint8_t t) {
  if (t > 12) return 0;
  return readRegister16(MPR121_FILTDATA_0L + t*2);
}

uint16_t  Adafruit_MPR121::baselineData(uint8_t t) {
  if (t > 12) return 0;
  uint16_t bl = readRegister8(MPR121_BASELINE_0 + t);
  return (bl << 2);
}

uint16_t  Adafruit_MPR121::touched(void) {
  uint16_t t = readRegister16(MPR121_TOUCHSTATUS_L);
  return t & 0x0FFF;
}

/*********************************************************************/


uint8_t Adafruit_MPR121::readRegister8(uint8_t reg) {
    Wire.beginTransmission(_i2caddr);
    Wire.write(reg);
    Wire.endTransmission(false);
    while (Wire.requestFrom(_i2caddr, 1) != 1);
    return ( Wire.read());
}

uint16_t Adafruit_MPR121::readRegister16(uint8_t reg) {
    Wire.beginTransmission(_i2caddr);
    Wire.write(reg);
    Wire.endTransmission(false);
    while (Wire.requestFrom(_i2caddr, 2) != 2);
    uint16_t v = Wire.read();
    v |=  ((uint16_t) Wire.read()) << 8;
    return v;
}


uint16_t Adafruit_MPR121::finishedAutoConfig(void) {
    uint8_t reg1 = readRegister8(MPR121_OUTOFRANGE_1);
	uint8_t reg2 = readRegister8(MPR121_OUTOFRANGE_2);
	
	Serial.print("\t 1=");
	Serial.print(reg1, HEX);
	Serial.print("\t 2=");
	Serial.print(reg2, HEX);
	
	if(reg1 == 0 && (reg2 & 0b00011111) == 0)
	{
		Serial.print("Return True (1)");
		return 1;
	}
	Serial.print("Return False (0)");
    return 0;
}



/**************************************************************************/
/*!
    @brief  Writes 8-bits to the specified destination register
*/
/**************************************************************************/
void Adafruit_MPR121::writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(_i2caddr);
    Wire.write((uint8_t)reg);
    Wire.write((uint8_t)(value));
    Wire.endTransmission();
}

float  Adafruit_MPR121::getCapacity(int id) {
//ADC low ist 217 -- 0,7V / 3,3V * 1024
//ADC high ist 806  -- 2,3V / 3,3V * 1024
  uint16_t ADC = filteredData(id);
  uint8_t T = readRegister8(MPR121_CHARGETIME_0 + (id/2));
  uint8_t I = readRegister8(MPR121_CHARGECURR_0 + id);
 
  if((id+1)%2)
  {
	T = T & 0b00000111;
  }
  else
  {
    T = T & 0b01110000;
	T = T >> 4;
  }
  I = I & 0b00011111;
  
  float time = pow(2 , T) / 4.0;
  float current = (float) I;
 
  float c = (current * time *1024.0) /(3.3 * (float)ADC);

  return c;
}
