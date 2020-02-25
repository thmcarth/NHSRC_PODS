/***************************************************
  This is a library for the Adafruit FONA 3G Cellular Module for HTTP 

  These displays use TTL Serial to communicate, 2 pins are required to
  interface
  
  
  
 ****************************************************/


#include "HTTPS_VIPER.h"
#include "Arduino.h"
#include "Adafrut_FONA.h"
#include "Encoder.h"


//back at EPA need to add in the encoder


HTTPS_VIPER::HTTPS_VIPER()
{
  Adafruit_FONA_3G fona = Adafruit_FONA_3G(FONA_RST)
}

int HTTPS_VIPER::getLength(char* content){ // use this to find body length, header length and total length of HTTP POST
int values = sizeof(content)/sizeof(content[0]);
return values;
}

    char* HTTPS_VIPER::build_POST( char* host,char* auth, char* body) //data_length is all the length of the body
	{
	int body_length = HTTPS_VIPER::getLength(body);
	char* header;       // Note! Order does not matter for an http Header
	char post[] = "POST /CAP/post HTTP/1.1\n";  
    char connection[] = "Connection: Keep-Alive\n";
	char *authorization;
	sprintf(authorization, "Authorization: Basic %s\n", auth);
	char *host_str;
	sprintf(host_str, "Host: %s\n", host);
	char *content;
	sprintf(content, "Content-Length: %i\n", body_length);
	int header_length = sprintf(header,"%s%s%s%s%s", post, host, authorization,content, connection);
	char* full_message;
	sprintf(full_message, "%s\r\n%s",header,body);

}


void HTTPS_VIPER::start_HTTP(Serial s){
	// turn HTTPS Stack on
  s.println("AT+CHTTPSSTART");
}

void HTTPS_VIPER::Open_HTTP(Serial s, char* host, char* port){
char* open;
sprintf(open, "AT+CHTTPSOPSE=\"%s\",%s,2", host, port);
s.println(open);
}

void HTTPS_VIPER::Send_HTTP(Serial s, char* post){
	char* send;
	int ready = 0;
	int length = HTTPS_VIPER::getLength(post);
	sprintf(send, "AT+CHTTPSSEND=%i", length);
	s.println(send);
	delay(10);
	
	if (ready){
	s.println(post);	
	}
	 
}

 int try_send (Serial s){
	if (s.available() > 0){
		
	for(int i = 0; i<300; i++){
		incomingByte = s.read();
	if (incomingByte == 62){ // we are waiting for the > symbol from the FONA 
	return 1;
	}
    delay (10);
	}
	}
	return 0;
}

void HTTPS_VIPER::Stop_HTTP(Serial s){
s.println("AT+CHTTPSSTOP");
}

void HTTPS_VIPER::Close_HTTP(Serial s){
	s.println("AT+CHTTPSCLSE");
}

