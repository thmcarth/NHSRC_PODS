/***************************************************
  This is a library for the Adafruit FONA 3G Cellular Module for HTTP 

  These displays use TTL Stream to communicate, 2 pins are required to
  interface
  
  @Author Hawes Collier
  @Credit to Adafruit and Ubidots for providing some example libraries
  @Owned by the USEPA 
  
 ****************************************************/

#ifndef HTTPS_VIPER_H
#define HTTPS_VIPER_H


#include <stdint.h>
#include <Arduino.h>
#include <Stream.h>
#include <stdio.h>  
namespace {
    
int identnum = 1;
char post_Data[2000];
int dataidx = 0;

}
class HTTPS_VIPER {

 public:
    HTTPS_VIPER(/*int fona_pin*/); //Initialize this class with the Reset Pin
    char* build_POST( char* host,char* auth, char* body); //host is the url, auth is username and password (base64 encoded),
	                                                      //body is the entire POST body that is beneath the message header
														//the body is built elsewhere
	char* build_body(int unit, char* GPS);
	void clearData();													  
    void start_HTTP();  // this must be first 
	void Send_HTTP(char* post);  //HTTPS OPEN first
	void Stop_HTTP();  // call when we want to end everything
	void Open_HTTP( char* host, char* port); //2nd 
	void Close_HTTP(); //call at the end when we are done
	bool is_error(); // call at the end of each AT command call. --> reads AT to see if ERROR was returned.
	char* read_FONA(); //read the AT command response
	bool init(Stream &port); //call to initialize serial to FONA (may not be necessary each time)
	void addInt(char* name,int value, char* units);
	void addFloat(char* name,float value, char* units);
	void addString(char* name,char* value, char* units);
	void check_GPRS();
 private:
   String charToString(char S[]);
   int getLength(char* content);
   void powerUpOrDown();  
   int try_send ();
   Stream *s;
};

#endif
