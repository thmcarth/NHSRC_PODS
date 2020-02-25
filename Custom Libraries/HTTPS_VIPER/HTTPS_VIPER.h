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
    explicit HTTPS_VIPER();
    char* build_POST( char* host,char* auth, char* body);
    void start_HTTP(Serial s);
	void Send_HTTP(Serial s, char* post);
	void Stop_HTTP(Serial s);
	void Open_HTTP(Serial s, char* host, char* port);
	void Close_HTTP(Serial s);
 private:
   int getLength(char* content);
   int try_send (Serial s);
};

#endif
