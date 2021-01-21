/*****************************************************************************
Master_Test.ino
Written By:  Bobby Schulz (schu3119@umn.edu)
Development Environment: PlatformIO 3.5.3
Hardware Platform: EnviroDIY Mayfly Arduino Datalogger
Software License: BSD-3.
  Copyright (c) 2017, Stroud Water Research Center (SWRC)
  and the EnviroDIY Development Team
Based on Wire Reader Master by Nicholas Zambetti <http://www.zambetti.com>
This sketch is designed to be loaded onto the master device in the system and reads from the slave tipping bucket counter which is loaded with the Salve_TippingBucket_Test.ino
firmware. This program simply reads the number of tips every 2 seconds, then prints this value to the serial monitor.
Connections to the salve device should be:
SCL -> Slave device SCL
SDA -> Slave device SDA
DISCLAIMER:
THIS CODE IS PROVIDED "AS IS" - NO WARRANTY IS GIVEN.
*****************************************************************************/


#include <Wire.h>

unsigned long UpdateRate = 4000; //Number of ms between serial prints, 4 seconds by default
uint8_t ADR = 8; //Address of slave device, 0x08 by default

void setup() {
  Wire.begin();        // join i2c bus (address optional for master)
  Serial.begin(9600);  // start serial for output
  Serial.print("Welcome to the Machine...\n\n"); //Genaric begin statment for monitor
}

void loop() {
  unsigned int tips = 0; //Used to measure the number of tips
  uint8_t Byte1 = 0; //Bytes to read then concatonate
  uint8_t Byte2 = 0;
  uint8_t c[2];
  Wire.requestFrom(ADR, 2);    // request 2 bytes from slave device #8
  int i = 0;
  while(Wire.available()) { // slave may send less than requested
    c[i++] = Wire.read();   // receive a byte as character
    //Serial.print(c[i]);        // print the character
  }

  Serial.println();
  //Byte1 = Wire.read();  //Read number of tips back
  //Byte2 = Wire.read();
  Serial.println(c[0]);
  Serial.println(c[1]);
  

  tips = ((c[1] << 8) | c[0]); //Concatenate bytes

  Serial.print("Number of Tips since last read = ");

  if (tips == 65535)
  {
      Serial.println("Sensor not connected");  // tips == 65535 if I2C isn't connectected
  }
  else
  {
      Serial.println(tips);  //Prints out tips to monitor
  }

  delay(UpdateRate); //Waits for next period
}
