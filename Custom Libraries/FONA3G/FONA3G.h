#ifndef FONA3G_H
#define FONA3G_H

#include "Arduino.h"
#include "Adafrut_FONA.h"
#include "SoftwareSerial.h"

#define FONA_RX 2
#define FONA_TX 3
#define FONA_RST 4
//This header will need to be updated 
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);
char replybuffer[255];

class FONA3G
{
private:
	char readBlocking();
    void flushSerial(Serial s);
	uint16_t readnumber();
	
public:
	void DeleteSMS();
	void sendSMS();
	void ReadWebsiteURL();
	void TurnOnGPRS();
	void TurnOffGPRS();
};
#endif