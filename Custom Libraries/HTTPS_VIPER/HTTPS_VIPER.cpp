/***************************************************
  This is a library for the Adafruit FONA 3G Cellular Module for HTTP 

  These displays use TTL Stream to communicate, 2 pins are required to
  interface
  
  @Author Hawes Collier
  @Credit to Adafruit and Ubidots for providing some example libraries
  @Owned by the USEPA 
  
 ****************************************************/


#include "HTTPS_VIPER.h"
#include "Arduino.h"
#include "Adafruit_FONA.h"
#include "Encoder.h" // if we need to add in dynamic 64 base encoding
using namespace std;



HTTPS_VIPER::HTTPS_VIPER(int fona_pin)
{
  Adafruit_FONA_3G fona = Adafruit_FONA_3G(fona_pin);
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

String HTTPS_VIPER::charToString(char S[])
{
 
 String rc(S);
 
}

void HTTPS_VIPER::start_HTTP(){
	// turn HTTPS Stack on
  s->println("AT+CHTTPSSTART");
}

void HTTPS_VIPER::Open_HTTP(char* host, char* port){
char* open;
sprintf(open, "AT+CHTTPSOPSE=\"%s\",%s,2", host, port);
s->println(open);
}

bool HTTPS_VIPER::init(Stream &port) {
    s = &port;
   //powerUpOrDown(); //placeholder for function to turn on/off GPRS shield
    return true;
}

char* HTTPS_VIPER::read_FONA(){
	//modeled off of Ubidots read data 
	int timeout = 2000;
  int index = 0;
  char buffer[1000];
    while (timeout--) {
    if (index >= 1000) {
      break;
    }
	 while (s->available()) {
      char c =  s->read();
      if (c == '\r') continue;
      if (c == 0xA) {
        if (index == 0)   // the first 0x0A is ignored
          continue;
      }
      buffer[index] = c;
      index++;
    }
	if (timeout == 0) {
      break;
				}
		delay(1);
			}
			buffer[index] = '\0';  // null term
			return buffer;
		}
bool HTTPS_VIPER::is_error(){
	//modeled off of Ubidots read data 
	int timeout = 2000;
  int index = 0;
  char buffer[1000];
    while (timeout--) {
    if (index >= 1000) {
      break;
    }
	 while (s->available()) {
      char c =  s->read();
      if (c == '\r') continue;
      if (c == 0xA) {
        if (index == 0)   // the first 0x0A is ignored
          continue;
      }
      buffer[index] = c;
      index++;
    }
	if (timeout == 0) {
      break;
				}
		delay(1);
			}
			buffer[index] = '\0';  // null term
			String check = HTTPS_VIPER::charToString(buffer);
			if (check.equals("ERROR"))
				return true;
			else 
				return false;
		}
	

/*
char* Ubidots::readData(uint16_t timeout) {
  uint16_t replyidx = 0;
  char replybuffer[500];
  while (timeout--) {
    if (replyidx >= 500) {
      break;
    }
    while (client->available()) {
      char c =  client->read();
      if (c == '\r') continue;
      if (c == 0xA) {
        if (replyidx == 0)   // the first 0x0A is ignored
          continue;
      }
      replybuffer[replyidx] = c;
      replyidx++;
    }

    if (timeout == 0) {
      break;
    }
    delay(1);
  }
  replybuffer[replyidx] = '\0';  // null term
  if (_debug) {
      Serial.println("Response of GPRS:");
      Serial.println(replybuffer);
  }

  while (client->available()) {
    client->read();
  }
  if (strstr(replybuffer,"NORMAL POWER DOWN")!=NULL) {
    powerUpOrDown();
  }
  return replybuffer;
}
*/

void HTTPS_VIPER::Send_HTTP(char* post){
	char* send;
	int ready = 0;
	int length = HTTPS_VIPER::getLength(post);
	sprintf(send, "AT+CHTTPSSEND=%i", length);
	s->println(send);
	delay(10);
	
	if (ready){
	s->println(post);	
	}
	 
}

 int HTTPS_VIPER::try_send (){
	if (s->available() > 0){
		
	for(int i = 0; i<300; i++){
		char incomingByte = s->read();
	if (incomingByte == 62){ // we are waiting for the > symbol from the FONA 
	return 1;
	}
    delay (10);
	}
	}
	return 0;
}

void HTTPS_VIPER::Stop_HTTP(){
s->println("AT+CHTTPSSTOP");
}

void HTTPS_VIPER::Close_HTTP(){
	s->println("AT+CHTTPSCLSE");
}

