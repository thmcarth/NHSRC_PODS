//Version 1.0

//Tested on Teensy3.5 with HydraProbe II (Stevens)

//This library is for the HydraProbe II Soil Moisture Sensor
//Returns temperature, soil moisture and conductivity readings from a HydraProbe II sensor
//This library uses SDI-12 communications and the SDI12 library 

//## License
//The SDI12 library code is released under the GNU Lesser Public License (LGPL 2.1) -- See [LICENSE-examples.md](https://github.com/EnviroDIY/Arduino-SDI-12/blob/master/LICENSE) file for details.

// the DataPin connected is configurable in the main call.




//Functions
// .begin(int datapin,int address) -- starts up SDI communication on the pin defined in the HydraProbe call (ex: HydraProbe hydraProbe(31)). Must pass sensor address (default is 0)
// .getHPStatus() -- returns boolean value. true if sensor responded appropriately to the configuration request during .begin, false if not
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
#include <HydraProbe.h>

float temp,moisture,conductivity;
float temp2,moisture2,conductivity2;
HydraProbe moistureSensor;  //define data Pin in Header
HydraProbe moistureSensor2;
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

if (moistureSensor.getHPStatus())
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
  Serial.println("Could not communicate with HydraProbe (0).");
  Serial.println("Check Connection.");
}

Serial.println("----------------------------------------");
Serial.println("----------------------------------------");
Serial.println("----------------------------------------");
Serial.println("---------------Sensor 2-----------------");
 
if (moistureSensor2.getHPStatus())
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
  Serial.println("Could not communicate with HydraProbe (1).");
  Serial.println("Check Connection.");
}
}
//-------------------------------------------------------------------------------
*/

#define hp_dataPin 30
#ifndef HydraProbe_h
#define HydraProbe_h
#include "Arduino.h"
#include "SDI12.h"

class HydraProbe
{
 public:
  char sdiResponse[50];
  String myCommand;
  float _hp_temp;
  float _hp_moisture;
  float _hp_conductivity;
 float _hp_permittivity;
  bool _HPReady;
  int _address;
  bool _hp_debug;
  HydraProbe()
{
  sdiResponse[50];
  myCommand = "";
  _hp_temp=0;
  _hp_moisture=0;
  _hp_conductivity=0;
  _hp_permittivity=0;
   _HPReady = false;
  _address=0;
  _hp_debug = false;
}
  
  void begin(int _SENSOR_ADDRESS);
  void parseResponse();
  void changeAddress(int to);
  bool getHPStatus();
  int getAddress();
  float getTemp();
  float getMoisture();
  float getConductivity();
  float getPermittivity();
  void debugOn();
  void debugOff();
  //int hp_dataPin;
  
  private:
  void takeMeasurement();
  
  
};

#endif

