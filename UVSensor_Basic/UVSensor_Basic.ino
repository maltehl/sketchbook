#include <Wire.h>
#include "Adafruit_VEML6070.h"

Adafruit_VEML6070 uv = Adafruit_VEML6070();

void setup() {
  Serial.begin(9600);
  Serial.println("VEML6070 Test");
  Wire.begin(D5,D6);
  uv.begin(VEML6070_1_T);  // pass in the integration time constant
}


void loop() {
  float value = (float)uv.readUV() / 10.0;
  Serial.print("UV light level: "); Serial.println(value);
  
  delay(1000);
}
