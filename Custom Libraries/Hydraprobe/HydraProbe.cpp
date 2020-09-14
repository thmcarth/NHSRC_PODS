/*
HydraProbe Library
*/


#include "SDI12.h"
#include <Arduino.h>
#include <stdlib.h>
#include <HydraProbe.h>



SDI12 mySDI12(hp_dataPin);

//int hp_dataPin;



int HydraProbe::getAddress()
{
return _address;
}

void HydraProbe::begin(int _SENSOR_ADDRESS) {
  _address = _SENSOR_ADDRESS;
  delay(50);
  mySDI12.begin();
  delay(200);
  Serial.println("Setting up Hydra Probe...");
  //first command to set up defaults
  myCommand = String(_SENSOR_ADDRESS) + "XM=HFJL!";
  //myCommand = String(_SENSOR_ADDRESS) + "A1!";
  if (_hp_debug) Serial.println(myCommand);     // echo command to terminal

  mySDI12.sendCommand(myCommand);
  delay(2000);                     // wait a while for a response

int z=0;
  while (mySDI12.available()) {  // build response string
    char c = mySDI12.read();
    if ((c != '\n') && (c != '\r')) {
      sdiResponse[z] = c;
      delay(5);
      ++z;
    }
  }
  if (sizeof(sdiResponse) > 1) Serial.println(sdiResponse); //write the response to the screen

 if (sdiResponse[0]-'0'== _SENSOR_ADDRESS && sdiResponse[3]=='J')
 {
  Serial.println("Success");
if (_hp_debug)  Serial.println("Setup complete...Ready for command measurement");
  _HPReady = true;
 }else
 {
  Serial.println("Failed");
  Serial.println("HydraProbe failed to initialize");
  _HPReady = false;
 }
   mySDI12.clearBuffer();
   
}


bool HydraProbe::getHPStatus(){
  if (_HPReady)
  {
    return true;
  }else
  {
    return false;
  }
}

void HydraProbe::takeMeasurement() {

//first command to take a measurement
  myCommand = String(_address) + "M!";
 if (_hp_debug) Serial.println(myCommand);     // echo command to terminal

  mySDI12.sendCommand(myCommand);
  delay(30);                     // wait a while for a response
  
int z=0;
  while (mySDI12.available()) {  // build response string
    char c = mySDI12.read();
    if ((c != '\n') && (c != '\r')) {
      sdiResponse[z] = c;
      delay(5);
      ++z;
  }
  }
 if (_hp_debug) if (sizeof(sdiResponse) > 1) Serial.println(sdiResponse); //write the response to the screen
  mySDI12.clearBuffer();
  Serial.println("Requested new measurement");


  delay(2000);                 // delay between taking reading and requesting data
  sprintf(sdiResponse,"%s","");           // clear the response string

}

void HydraProbe::changeAddress(int to) {


delay(2000);
//first command to take a measurement
  myCommand = String(_address) + "A" + String(to) + "!";
 if (_hp_debug) Serial.println(myCommand);     // echo command to terminal

  mySDI12.sendCommand(myCommand);
  delay(2000);                     // wait a while for a response
  
int z=0;
  while (mySDI12.available()) {  // build response string
    char c = mySDI12.read();
    if ((c != '\n') && (c != '\r')) {
      sdiResponse[z] = c;
      delay(5);
      ++z;
  }
  }
 Serial.print("Changed ID from ");Serial.print(_address);Serial.print(" to ");Serial.println(to);
 if (_hp_debug) if (sizeof(sdiResponse) > 1) Serial.println(sdiResponse); //write the response to the screen
_address = to;
  mySDI12.clearBuffer();
 


  delay(2000);                 // delay between taking reading and requesting data
  sprintf(sdiResponse,"%s","");           // clear the response string

}


float HydraProbe::getTemp()
{
return _hp_temp;  
}
float HydraProbe::getMoisture()
{
return _hp_moisture;  
}
float HydraProbe::getConductivity()
{
return _hp_conductivity;  
}
float HydraProbe::getPermittivity()
{
return _hp_permittivity;  
}
  





void HydraProbe::parseResponse(){
char tempIn[10];
char moistureIn[10];
char conductivityIn[10];
char permittivityIn[10];
int plusCount = 0;
int a=0;
int b=0;
int c=0;
int d=0;
_hp_temp = -9999;
_hp_moisture = -9999;
_hp_conductivity = -9999;
_hp_permittivity = -9999;

takeMeasurement();

//Request Data Response from last Measurement

  myCommand = String(_address) + "D0!";
 if (_hp_debug) Serial.println(myCommand);  // echo command to terminal

  mySDI12.sendCommand(myCommand);
  delay(2000);                     // wait a while for a response

 int  z=0;
  while (mySDI12.available()) {  // build response string
    char c = mySDI12.read();
    if ((c != '\n') && (c != '\r')) {
      sdiResponse[z] = c;
      delay(5);
      ++z;
  }
  }
  //Serial.println(z);
if (_hp_debug)
{
 	if (z > 1) 
	{
	Serial.println(sdiResponse); //write the response to the screen
	Serial.println("Response Received");
	}else
	{
	Serial.println("No response from Get Data Request");
	}

}
  mySDI12.clearBuffer();


//Serial.println(sizeof(sdiResponse));
for (int y=0 ; y<sizeof(sdiResponse) ; y++)
{
  char h = sdiResponse[y];
  if ( h =='+')
  {
    ++plusCount;
  }else
  {
    switch (plusCount){
      case 1:    //moisture
        if (a<sizeof(moistureIn))
      {
      moistureIn[a] = h;
      }
      ++a;
      break;
      
      case 2:    //temp
        if (b<sizeof(tempIn))
      {
      tempIn[b] = h;
      }
       ++b;
      break;
     
      
      case 3:    //conductivity
      if (c<sizeof(conductivityIn))
      {
      conductivityIn[c] = h;
      }
      ++c;
      break;
  case 4:    //permittivity
      if (c<sizeof(permittivityIn))
      {
      permittivityIn[d] = h;
      }
      ++d;
      break;
      default:;
      
    }
    
  }
}
_hp_temp = atof(tempIn);
_hp_permittivity = atof(permittivityIn);
_hp_moisture = atof(moistureIn);
_hp_conductivity = atof(conductivityIn);
if (_hp_debug)
{
  
  Serial.println(tempIn);
  Serial.println(a);
  Serial.println(b);
  Serial.println(c);
  Serial.println(moistureIn);
  Serial.println(conductivityIn);
  Serial.println(permittivityIn);
  
  Serial.print("Temp = ");Serial.print(_hp_temp);Serial.println(" deg C");
  Serial.print("Moisture = ");Serial.print(_hp_moisture*100);Serial.println(" %");
  Serial.print("Conductivity = ");Serial.print(_hp_conductivity);Serial.println(" S/m");
  Serial.print("Permittivity = ");Serial.println(_hp_permittivity);
}
  sprintf(tempIn,"%s","");           // clear the temp string
  sprintf(moistureIn,"%s","");           // clear the moisture string
  sprintf(conductivityIn,"%s","");           // clear the conductivity string
 sprintf(permittivityIn,"%s","");           // clear the permittivity string
 }

void HydraProbe::debugOn()
{
_hp_debug = true;
}


void HydraProbe::debugOff()
{
_hp_debug = false;
}

