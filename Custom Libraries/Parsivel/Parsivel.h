//Version 1.0

//Tested on Teensy3.5 with Parsivel II (Stevens)

//This library is for the Parsivel II Soil Moisture Sensor
//Returns temperature, soil moisture and conductivity readings from a Parsivel II sensor
//This library uses SDI-12 communications and the SDI12 library 

//## License
//The SDI12 library code is released under the GNU Lesser Public License (LGPL 2.1) -- See [LICENSE-examples.md](https://github.com/EnviroDIY/Arduino-SDI-12/blob/master/LICENSE) file for details.

// the DataPin connected is configurable in the main call.




//Functions
// .begin(int datapin,int address) -- starts up SDI communication on the pin defined in the Parsivel call (ex: Parsivel Parsivel(31)). Must pass sensor address (default is 0)
// .getpStatus() -- returns boolean value. true if sensor responded appropriately to the configuration request during .begin, false if not
// .parseResponse() -- parses the output from the sensor on the selected datapin. Need to run this before get methods.
// .getTemp() -- returns Temp reading (float)  degC-- must parse first
// .getMoisture() -- returns Moisture reading (float) % -- must parse first
// .changeAdress(int to) -- change address for specific sensor
// .getConductivity -- returns Conductivity reading (float) S/m -- must parse first

// .debugOn() -- turns on verbose serial output for diagnostics
// .debugOff() -- turns off verbose serial output

//Usage example
/*
//---------------------------------------------------------------
#include <Parsivel.h>

float temp,moisture,conductivity;
float temp2,moisture2,conductivity2;
Parsivel moistureSensor;  //define data Pin in Header
Parsivel moistureSensor2;
void setup() {
Serial.begin(9600);
delay(1000);
moistureSensor.debugOff(); //Can turn debug on or off to see verbose output (OR NOT)
moistureSensor.begin(0);
moistureSensor2.debugOff(); //Can turn debug on or off to see verbose output (OR NOT)
moistureSensor2.begin(1);
}

void loop(){
 delay(2000);
Serial.println("----------------------------------------");
Serial.println("----------------------------------------");
Serial.println("----------------------------------------");
Serial.println("---------------Sensor 1-----------------");

if (moistureSensor.getpStatus())
{
moistureSensor.parseResponse();


temp = moistureSensor.getTemp();
moisture = moistureSensor.getMoisture();
conductivity = moistureSensor.getConductivity();

Serial.print("Temp = ");
Serial.println(temp);
Serial.print("Moisture = ");
Serial.println(moisture);
Serial.print("Conductivity = ");
Serial.println(conductivity);
Serial.println("");
}
else
{
  Serial.println("Could not communicate with Parsivel (0).");
  Serial.println("Check Connection.");
}

Serial.println("----------------------------------------");
Serial.println("----------------------------------------");
Serial.println("----------------------------------------");
Serial.println("---------------Sensor 2-----------------");
 
if (moistureSensor2.getpStatus())
{
moistureSensor2.parseResponse();

 
temp2 = moistureSensor2.getTemp();
moisture2 = moistureSensor2.getMoisture();
conductivity2 = moistureSensor2.getConductivity();

Serial.print("Temp 2 = ");
Serial.println(temp2);
Serial.print("Moisture 2 = ");
Serial.println(moisture2);
Serial.print("Conductivity 2 = ");
Serial.println(conductivity2);
Serial.println("");
}
else
{
  Serial.println("Could not communicate with Parsivel (1).");
  Serial.println("Check Connection.");
}
}
//-------------------------------------------------------------------------------
*/

#define p_dataPin 2
#ifndef Parsivel_h
#define Parsivel_h
#include "Arduino.h"
#include "SDI12.h"

class Parsivel
{
 public:
  char sdiResponse[50];
  String myCommand;
  float _p_intensity;
  float _p_temp;
  float _p_moisture;
  float _p_conductivity;
 float _p_permittivity;
  bool _pReady;
  int _address;
  bool _p_debug;
  Parsivel()
{
  sdiResponse[50];
  myCommand = "";
  _p_temp=0;
  _p_moisture=0;
  _p_conductivity=0;
  _p_permittivity=0;
   _pReady = false;
  _address=0;
  _p_debug = false;
}
  
  void begin(int _SENSOR_ADDRESS);
  void parseResponse();
  void changeAddress(int to);
  bool getpStatus();
  int getAddress();
  float getTemp();
  float getMoisture();
  float getConductivity();
  float getPermittivity();
  float getIntensity();
  void debugOn();
  void debugOff();
  //int p_dataPin;
  
  private:
  void takeMeasurement();
  void takeMeasurement_2();
  
  
};

#endif

