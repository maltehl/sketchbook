#include <SoftWire.h>
#include "Adafruit_VEML6070.h"

Adafruit_VEML6070 uv = Adafruit_VEML6070();
#define SDA_PIN D4
#define SCL_PIN D5

SoftWire myWire(SDA_PIN, SCL_PIN);

void setup() {
  Serial.begin(9600);
  Serial.println("VEML6070 Test");
  myWire.begin();
  uv.begin(VEML6070_1_T, &myWire);  // pass in the integration time constant
}


void loop() {
  Serial.print("UV light level: "); Serial.println(uv.readUV());
  
  delay(1000);
}
