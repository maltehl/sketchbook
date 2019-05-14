#include <Servo.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Helios - Version: Latest 
#include <Helios.h>




Helios helios;
Servo myservo;
#define OLED_RESET 0  // GPIO0
Adafruit_SSD1306 display(OLED_RESET);
 
#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2
 
 
#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH  16

//float dAzimuth;
//float dElevation;

  byte second=0;
  byte minute=0;
  byte hour=13;
  byte dayOfMonth=6;
  byte month=6;
  byte year=18;


//DM
//  float lon = 53.926649;
//  float lat = 13.014776;
//Dassow
  float lon = 53.904846; 
  float lat = 11.001819;


  //float tanalt;
  //float sinazi;
  //float angle; 
  float angledec;   

void setup()
{
  Serial.begin(9600); 
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
 
  
  myservo.attach(D4);
  myservo.write(0);
  display.println("0 deg");
  display.display();
  delay(2000);
  display.clearDisplay();
  myservo.write(180);
  display.println("180 deg");
  display.display();
  delay(2000);
  
  
  //uncomment following to set the timer chip
  //Once done, don't forget to comment out again, otherwise
  //you get the same time with each startup ;-)

}



void loop()
{
  char sTime[50];
 //16.36667,48.2 is for 48°12'N, 16°22'O (latitude and longitude of Vienna)
  //Time in GMT
  helios.calcSunPos(year,month,dayOfMonth,hour-2, minute,second,lat,lon); //-2 ->GMT
  sprintf(sTime,"%d+%d+%d - %d:%d:%d ->",dayOfMonth,month,year,hour,minute,second);
  Serial.print(sTime);
  //dAzimuth=helios.dAzimuth;
  //show("dAzimuth",dAzimuth,true);
  //dElevation=helios.dElevation;
  //show("dElevation",dElevation,true);

  //tanalt = tan(dElevation/180*PI);
  //show("tanalt",tanalt,true);
  
  //sinazi = sin(dAzimuth/180*PI);
  //show("sinazi",sinazi,true);
  
  //angle = atan(tan(dElevation/180*PI)/sin(dAzimuth/180*PI));
  //show("Angle",angle,true);
  if(helios.dElevation > 0)
    angledec = atan(tan(helios.dElevation/180*PI)/sin(helios.dAzimuth/180*pi)) / pi * 180;
  else
    angledec = 0;
  
  
  minute += 1;
  if(minute >= 60)
  {
    hour += 1;
    minute -= 60;
  }
  if(hour>=24)
  {
    dayOfMonth +=1;
    hour -= 24;
  }

  if(angledec < 0)
  {
    angledec = - angledec;
  }
  else
  {
    angledec = 180 - angledec;  
  }
  
  show("AngleDec",angledec,true);
  myservo.write(angledec);
  display.clearDisplay();
  display.setCursor(0,0);
  display.println(angledec);
  display.display();
  
  delay(60000);
  
}



void show(char nameStr[], double val, boolean newline) {
  Serial.print(nameStr);  
  Serial.print("=");
  if (newline)
       Serial.println(val);
  else Serial.print(val);
}
