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

int HTTPS_VIPER::getLength(char* content){
int values = sizeof(content)/sizeof(content[0]);
return values;
}

    char* HTTPS_VIPER::build_POST(int data_length, char* host,char* auth, char* body)
	{
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
	char* full_message;
	sprintf(full_message, "%s", header);

}


void HTTPS_VIPER::start_HTTP(Serial s){
	// turn HTTPS Stack on
  // how do i send serial and AT commands?
  s.println("AT+CHTTPSSTART");
	
}

void HTTPS_VIPER::Open_HTTP(Serial s, char* host, char* port){
char* open;
sprintf(open, "AT+CHTTPSOPSE=\"%s\",%s,2", host, port);
s.println(open);
}

void HTTPS_VIPER::Send_HTTP(Serial s, int length, char* post){
	char* send;
	int ready = 0;
	sprintf(send, "AT+CHTTPSSEND=%i", length);
	s.println(send);
	delay(10);
	 if (Serial.available() > 0) {
    incomingByte = s.read();
	for(int i = 0; i<100; i++){
	if (incomingByte == 62) // we are waiting for the > symbol from the FONA 
	ready = 1;
    delay (10);
	}
	if (ready){
	s.print(post);	
	}
	 }
}

void HTTPS_VIPER::Stop_HTTP(Serial s){
s.println("AT+CHTTPSSTOP");
}

void HTTPS_VIPER::Close_HTTP(Serial s){
	s.println("AT+CHTTPSCLSE");
}


/*
 Serial.println(AT+CGREG);

  // Check Packet Domain attach or detach
  Serial.println(AT+CGATT?);

  //Set the PDP Contect Profile
  Serial.println(AT+CGSOCKCONT=1,"IP","live.vodafone.com");

  Serial.println(AT+CGDCONT=1,"IP","live.vodafone.com","0.0.0.0",0,0);
  Serial.println(AT+CGAUTH=1,1,"","");
  Serial.println(AT+CGATT=1 );

  // Activate or deactivate the specified PDP contect(s)
  Serial.println(AT+CGACT=1,1 );
  Serial.println(AT+CSOCKSETPN=1 );

  // Check network registration status
  Serial.println(AT+CGREG);

  // Check Packet Domain attach or detach
  Serial.println(AT+CGATT?);


   getHTMLPage();

*/


