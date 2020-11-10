/*
Parsivel Library
*/


#include "SDI12.h"
#include <Arduino.h>
#include <stdlib.h>
#include <Parsivel.h>



SDI12 mySDI12(p_dataPin);

//int p_dataPin;



int Parsivel::getAddress()
{
return _address;
}

void Parsivel::begin(int _SENSOR_ADDRESS) {
  _address = _SENSOR_ADDRESS;
  delay(50);
  mySDI12.begin();
  delay(200);
  Serial.println("Setting up Parsivel...");
  //first command to set up defaults
 // myCommand = String(_SENSOR_ADDRESS) + "XM=HFJL!";
  myCommand = String(_SENSOR_ADDRESS) + "!";
  if (_p_debug) Serial.println(myCommand);     // echo command to terminal

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

 if (sdiResponse[0]-'0'== _SENSOR_ADDRESS)
 {
  Serial.println("Success");
if (_p_debug)  Serial.println("Setup complete...Ready for command measurement");
  _pReady = true;
 }else
 {
  Serial.println("Failed");
  Serial.println("Parsivel failed to initialize: ");
  Serial.println(sdiResponse);
  _pReady = false;
 }
   mySDI12.clearBuffer();
   
}


bool Parsivel::getpStatus(){
  if (_pReady)
  {
    return true;
  }else
  {
    return false;
  }
}

void Parsivel::takeMeasurement() {

//first command to take a measurement
  myCommand = String(_address) + "M!";
 if (_p_debug) Serial.println(myCommand);     // echo command to terminal

  mySDI12.sendCommand(myCommand);
  delay(100);                     // wait a while for a response
  
int z=0;
  while (mySDI12.available()) {  // build response string
    char c = mySDI12.read();
    if ((c != '\n') && (c != '\r')) {
      sdiResponse[z] = c;
      delay(5);
      ++z;
  }
  }
  delay(9000);   
 if (_p_debug) if (sizeof(sdiResponse) > 1) Serial.println(sdiResponse); //write the response to the screen
  mySDI12.clearBuffer();
  Serial.println("Requested new measurement");


  delay(2000);                 // delay between taking reading and requesting data
  sprintf(sdiResponse,"%s","");           // clear the response string

}


void Parsivel::takeMeasurement_2() {

//first command to take a measurement
  myCommand = String(_address) + "M1!";
 if (_p_debug) Serial.println(myCommand);     // echo command to terminal

  mySDI12.sendCommand(myCommand);
  delay(100);                     // wait a while for a response
  
int z=0;
  while (mySDI12.available()) {  // build response string
    char c = mySDI12.read();
    if ((c != '\n') && (c != '\r')) {
      sdiResponse[z] = c;
      delay(5);
      ++z;
  }
  }
  delay(1000);   
 if (_p_debug) if (sizeof(sdiResponse) > 1) Serial.println(sdiResponse); //write the response to the screen
  mySDI12.clearBuffer();
  Serial.println("Requested new measurement");


  delay(2000);                 // delay between taking reading and requesting data
  sprintf(sdiResponse,"%s","");           // clear the response string

}


void Parsivel::changeAddress(int to) {


delay(2000);
//first command to take a measurement
  myCommand = String(_address) + "A" + String(to) + "!";
 if (_p_debug) Serial.println(myCommand);     // echo command to terminal

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
 if (_p_debug) if (sizeof(sdiResponse) > 1) Serial.println(sdiResponse); //write the response to the screen
_address = to;
  mySDI12.clearBuffer();
 


  delay(2000);                 // delay between taking reading and requesting data
  sprintf(sdiResponse,"%s","");           // clear the response string

}

float Parsivel::getIntensity()
{
	return _p_intensity;
}
float Parsivel::getTemp()
{
return _p_temp;  
}
float Parsivel::getMoisture()
{
return _p_moisture;  
}
float Parsivel::getConductivity()
{
return _p_conductivity;  
}
float Parsivel::getPermittivity()
{
return _p_permittivity;  
}
  





void Parsivel::parseResponse1(){
char Intensity[10];
char tempIn[10];
char moistureIn[10];
char conductivityIn[10];
char permittivityIn[10];
int plusCount = 0;
int a=0;
int b=0;
int c=0;
int d=0;

takeMeasurement();

//Request Data Response from last Measurement

  myCommand = String(_address) + "D0!";
 if (_p_debug) Serial.println(myCommand);  // echo command to terminal

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
  //myCommand = String(_address) + "D1!";
  //Serial.println(z);
if (_p_debug)
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


Serial.println(sizeof(sdiResponse));
for (int y=0 ; y<sizeof(sdiResponse) ; y++)
{
	
	
  /* char h = sdiResponse[y];
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
    
  } */
}

if (_p_debug)
{
  
  Serial.println(tempIn);
  Serial.println(a);
  Serial.println(b);
  Serial.println(c);
  Serial.println(moistureIn);
  Serial.println(conductivityIn);
  Serial.println(permittivityIn);
  
  Serial.print("Temp = ");Serial.print(_p_temp);Serial.println(" deg C");
  Serial.print("Moisture = ");Serial.print(_p_moisture*100);Serial.println(" %");
  Serial.print("Conductivity = ");Serial.print(_p_conductivity);Serial.println(" S/m");
  Serial.print("Permittivity = ");Serial.println(_p_permittivity);
}
  sprintf(tempIn,"%s","");           // clear the temp string
  sprintf(moistureIn,"%s","");           // clear the moisture string
  sprintf(conductivityIn,"%s","");           // clear the conductivity string
 sprintf(permittivityIn,"%s","");           // clear the permittivity string
 }

void Parsivel::debugOn()
{
_p_debug = true;
}


void Parsivel::debugOff()
{
_p_debug = false;
}

