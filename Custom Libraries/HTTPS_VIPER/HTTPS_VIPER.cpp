/***************************************************
  This is a library for the Adafruit FONA 3G Cellular Module for HTTP 

  These displays use TTL Stream to communicate, 2 pins are required to
  interface
  
  @Author Hawes Collier
  @Credit to Adafruit and Ubidots for providing some example libraries
  @Owned by the USEPA 
  
 ****************************************************/
#include <avr/pgmspace.h>
#ifdef ARDUINO_ARCH_AVR
#include <avr/wdt.h>
#else
#define wdt_reset()
#endif

#include "HTTPS_VIPER.h"
#include "Arduino.h"
#include "Adafruit_FONA.h"
//#include "Encoder.h" // if we need to add in dynamic 64 base encoding
//using namespace std;



HTTPS_VIPER::HTTPS_VIPER(/*int fona_pin*/)
{
 //Adafruit_FONA_3G fona = Adafruit_FONA_3G(fona_pin);
}

void HTTPS_VIPER::clearData()
{

Serial.println("In clear data");
sprintf(post_Data,"%s","");
dataidx = 0;

}

int HTTPS_VIPER::getLength(char* content){ // use this to find body length, header length and total length of HTTP POST
int values = sizeof(content);
return values;
}

char* HTTPS_VIPER::build_body(int unit, char* GPS){
	int tries = 0;
    char allData[3000];
    char str_values[10];
    char postS[500];
    char tempS[100];
    char ident[25];
    char sourceS[30];
    char footer[30];
    char header[500];

sprintf(ident,"%i",identnum);
++identnum;


 sprintf(sourceS, "%s%i%s", "EPA-WET-BOARD ",unit, ",0,0");  // Can change the zeros to increment in a cascading format to add more devices                                                                         
 sprintf(postS, "%s%s%s%s%s%s%s%s%s", "<identifier>", ident, "</identifier>",  "<source>", sourceS, "</source>", "<info><area><circle>",0/*GPS,*/, "</circle></area><headline>");
 sprintf(header,"%s","<?xml version=\"1.0\" encoding=\"utf-8\"?><alert xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns=\"urn:oasis:names:tc:emergency:cap:1.1\">");
 sprintf(footer,"%s","</headline></info></alert>");
 sprintf(allData,"%s%s%s%s",header,postS,post_Data,footer);
 Serial.print("allData is - ");Serial.println(allData);
 return allData;
}

char* HTTPS_VIPER::build_POST( char* host,char* auth, char* body) //data_length is all the length of the body
	{
		
	char* b = (char *) malloc(sizeof(char) * 3000);
	int body_length;
	body_length = sprintf(b,"%s",body);
	
	char* header = (char *) malloc(sizeof(char) * 300);       // Note! Order does not matter for an http Header
	char post[] = "POST /CAP/post HTTP/1.1\n";  
    char connection[] = "Connection: Keep-Alive\n";
	char *authorization = (char *) malloc(sizeof(char) * 130);
	sprintf(authorization, "Authorization: Basic %s\n", auth);
	
	char *host_str  = (char *) malloc(sizeof(char) * 70);
	sprintf(host_str, "Host: %s\n", host);
	
	char *content  = (char *) malloc(sizeof(char) * 70);
	sprintf(content, "Content-Length: %i\n", body_length);
	int header_length = sprintf(header,"%s%s%s%s%s", post, host, authorization,content, connection);
	
	char* full_message  = (char *) malloc(sizeof(char) * 2000);
	sprintf(full_message, "%s\r\n%s",header,body);
	free(b);
	free(header);
	free(authorization);
	free(host_str);
	free(content);
	//free();
    return full_message;
	}
	
void HTTPS_VIPER::addInt(char* name,int value, char* units)
{
if (dataidx == 0)
{
sprintf(post_Data,"%s%s;%i;%s;Green",post_Data,name,value,units);
}else
{
sprintf(post_Data,"%s;%s;%i;%s;Green",post_Data,name,value,units);
}
++dataidx;
}

void HTTPS_VIPER::addFloat(char* name,float value, char* units)
{
if (dataidx == 0)
{
sprintf(post_Data,"%s%s;%f;%s;Green",post_Data,name,value,units);
}else
{
sprintf(post_Data,"%s;%s;%f;%s;Green",post_Data,name,value,units);
}
++dataidx;
}

void HTTPS_VIPER::addString(char* name,char* value, char* units)
{
if (dataidx == 0)
{
sprintf(post_Data,"%s%s;%s;%s;Green",post_Data,name,value,units);
}else
{
sprintf(post_Data,"%s;%s;%s;%s;Green",post_Data,name,value,units);
}
++dataidx;
}

String HTTPS_VIPER::charToString(char S[])
{
 
 String rc(S);
 return rc;
}

void HTTPS_VIPER::start_HTTP(){
	// turn HTTPS Stack on
  s->println("AT+CHTTPSSTART");
}

void HTTPS_VIPER::Open_HTTP(char* host, char* port){
char open[100];
//Serial.println(F("Error with open"));
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

void HTTPS_VIPER::Send_HTTP(char* post){
	char* send = (char *) malloc(sizeof(char) * 2000);
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

void HTTPS_VIPER::check_GPRS(){
	s->println("AT+CGATT=?");
}