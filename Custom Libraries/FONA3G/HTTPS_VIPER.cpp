/***************************************************
  This is a library for the Adafruit FONA 3G Cellular Module for HTTP 

  These displays use TTL Serial to communicate, 2 pins are required to
  interface
  
 ****************************************************/


#include "HTTPS_VIPER.h"
#include "Arduino.h"
#include "Adafrut_FONA.h"



//back at EPA need to add in the encoder


HTTPS_VIPER::HTTPS_VIPER()
{
  
}

int HTTPS_VIPER::getLength(char* content){
int values = sizeof(content);
return values;
}

char* HTTPS_VIPER::buildHTTP_POST(int data_length, char* host,char* auth){
	char* header;
	char post[] = "POST /CAP/post HTTP/1.1\n";  // Note! Order does not matter for an http Header
    char connection[] = "Connection: Keep-Alive\n";
	char *authorization;
	sprintf(authorization, "Authorization: Basic %s\n", auth);
	char *host_str;
	sprintf(host_str, "Host: %s\n", host);
	char *content;
	sprintf(content, "Content-Length: %s\n", data_length);
	sprintf(header,"%s%s%s%s%s", post, host, authorization,content, connection);
}

