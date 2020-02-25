/*
  HTTPS Library by Hawes Collier 
  Used for communicating with 
*/

#ifndef HTTPS_VIPER_H
#define HTTPS_VIPER_H


#include <stdint.h>
#include <Arduino.h>
#include <Stream.h>
#include <stdio.h>  

class HTTPS_VIPER {

 public:
    HTTPS_VIPER(int fona_pin); //Initialize this class with the Reset Pin
    char* build_POST( char* host,char* auth, char* body); //host is the url, auth is username and password (base64 encoded),
	                                                      //body is the entire POST body that is beneath the message header
														  //the body is built elsewhere
    void start_HTTP(Stream s);
	void Send_HTTP(Stream s, char* post);
	void Stop_HTTP(Stream s);
	void Open_HTTP(Stream s, char* host, char* port);
	void Close_HTTP(Stream s);
 private:
   int getLength(char* content);
   int try_send (Stream s);
   Stream *serial;
};

#endif
