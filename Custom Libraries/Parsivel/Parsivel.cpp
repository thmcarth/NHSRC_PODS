/*
Parsivel Library
*/


#include "SDI12.h"
#include <Arduino.h>
#include <stdlib.h>
#include <Parsivel.h>
#include <string>


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
	while (mySDI12.available()) {  // build response string
    char c = mySDI12.read();
    if ((c != '\n') && (c != '\r')) {
      sdiResponse[z] = c;
      delay(5);
      ++z;
  }
  }  
 if (_p_debug) if (sizeof(sdiResponse) > 1) Serial.println(sdiResponse); //write the response to the screen
  mySDI12.clearBuffer();
  Serial.println("Requested new measurement");

               // delay between taking reading and requesting data
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

String Parsivel::getIntensity()
{
	return _p_intensity;
}
String Parsivel::getTemp()
{
return _p_temp;  
}
String Parsivel::getMoisture()
{
return _p_moisture;  
}
String Parsivel::getConductivity()
{
return _p_conductivity;  
}
String Parsivel::getPermittivity()
{
return _p_permittivity;  
}
  





void Parsivel::parseResponse1(){
string intensity;
string accumulate;
string particleNum;
int commaNum = 0;
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

 int data_len = 0;
 int data_pos = 0;
 int last_comma =0;
Serial.println(sizeof(sdiResponse));
string data = sdiResponse;
for (int y=0 ; y < data.length(); y++)
{
	
   char h = data.at(y)
 
  if ( h ==','){ ++commaNum;
  
    switch (commaNum){
	  
	  case 1:   
	  intensity = data.substr(data_pos,data_len);
	  data_len = 0;
	  last_comma = y;
	  data_pos = y+1
      break;
      case 2:    //temp
	  
       accumulate = data.substr(data_pos,data_len);
	    data_len = 0;
	  last_comma = y;
	  data_pos = y+1
      break;
     
      default:
	  
	  break;
      
    }
	
  }
  else
    data_len++;
  }
  _p_intensity = intensity;
  _p_accumulate - accumulate;
 

if (_p_debug)
{
  
  Serial.println(intensity);
  Serial.println(accumulate);
 
  
  Serial.print("Intensity = ");Serial.print(_p_temp);Serial.println(" mm/h");
  Serial.print("Accumulate = ");Serial.print(_p_moisture);Serial.println(" mm");
 
}
  sprintf(intensity,"%s","");           // clear the intensity string
  sprintf(accumulate,"%s","");           // clear the accumulate string
  sprintf(particleNum,"%s","");           // clear the particlenum string
  sprintf(sdiResponse,"%s","");
  
   myCommand = String(_address) + "D1!";
   Serial.println(z);
   if (_p_debug) Serial.println(myCommand);  // echo command to terminal
   mySDI12.sendCommand(myCommand);
   delay(100);                     // wait a while for a response
   
   int  z=0;
  while (mySDI12.available()) {  // build response string
    char c = mySDI12.read();
    if ((c != '\n') && (c != '\r')) {
      sdiResponse[z] = c;
      delay(5);
      ++z;
  }
  }
   mySDI12.clearBuffer();

 int data_len = 0;
 int data_pos = 0;
 int last_comma =0;
Serial.println(sizeof(sdiResponse));
string data = sdiResponse;
commaNum = 0;
for (int y=0 ; y < data.length(); y++)
{
	
   char h = data.at(y)
 
  if ( h ==','){ 
  ++commaNum;
    switch (commaNum){
	  
	  case 1:   
	  last_comma = y;
	  data_pos = y+1
	  data_len = 0;
      break;
      
      case 2:    //particleNum
      particleNum = data.substr(data_pos,data_len);
	  data_len = 0;
	  last_comma = y;
	  data_pos = y+1
      break;
     
      default:
	  
	  break;
      
    }
  }
  else 
  data_len++;
  } 
  
  _p_particleNum = particleNum;
 sprintf(particleNum,"%s","");           // clear the particlenum string
  sprintf(sdiResponse,"%s","");
}   


void Parsivel::parseResponse2(){
string intensity;
string accumulate;
string particleNum;
int commaNum = 0;
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

 int data_len = 0;
 int data_pos = 0;
 int last_comma =0;
Serial.println(sizeof(sdiResponse));
string data = sdiResponse;
for (int y=0 ; y < data.length(); y++)
{
	
   char h = data.at(y)
 
  if ( h ==','){ ++commaNum;
  
    switch (commaNum){
	  
	  case 1:   
	  intensity = data.substr(data_pos,data_len);
	  data_len = 0;
	  last_comma = y;
	  data_pos = y+1
      break;
      case 2:    //temp
	  
       accumulate = data.substr(data_pos,data_len);
	    data_len = 0;
	  last_comma = y;
	  data_pos = y+1
      break;
     
      default:
	  
	  break;
      
    }
	
  }
  else
    data_len++;
  }
  _p_intensity = intensity;
  _p_accumulate - accumulate;
 

if (_p_debug)
{
  
  Serial.println(intensity);
  Serial.println(accumulate);
 
  
  Serial.print("Intensity = ");Serial.print(_p_temp);Serial.println(" mm/h");
  Serial.print("Accumulate = ");Serial.print(_p_moisture);Serial.println(" mm");
 
}
  sprintf(intensity,"%s","");           // clear the intensity string
  sprintf(accumulate,"%s","");           // clear the accumulate string
  sprintf(particleNum,"%s","");           // clear the particlenum string
  sprintf(sdiResponse,"%s","");
  
   myCommand = String(_address) + "D1!";
   Serial.println(z);
   if (_p_debug) Serial.println(myCommand);  // echo command to terminal
   mySDI12.sendCommand(myCommand);
   delay(100);                     // wait a while for a response
   
   int  z=0;
  while (mySDI12.available()) {  // build response string
    char c = mySDI12.read();
    if ((c != '\n') && (c != '\r')) {
      sdiResponse[z] = c;
      delay(5);
      ++z;
  }
  }
   mySDI12.clearBuffer();

 int data_len = 0;
 int data_pos = 0;
 int last_comma =0;
Serial.println(sizeof(sdiResponse));
string data = sdiResponse;
commaNum = 0;
for (int y=0 ; y < data.length(); y++)
{
	
   char h = data.at(y)
 
  if ( h ==','){ 
  ++commaNum;
    switch (commaNum){
	  
	  case 1:   
	  last_comma = y;
	  data_pos = y+1
	  data_len = 0;
      break;
      
      case 2:    //particleNum
      particleNum = data.substr(data_pos,data_len);
	  data_len = 0;
	  last_comma = y;
	  data_pos = y+1
      break;
     
      default:
	  
	  break;
      
    }
  }
  else 
  data_len++;
  } 
  
  _p_particleNum = particleNum;
 sprintf(particleNum,"%s","");           // clear the particlenum string
  sprintf(sdiResponse,"%s","");
}   



void Parsivel::debugOn()
{
_p_debug = true;
}


void Parsivel::debugOff()
{
_p_debug = false;
}

