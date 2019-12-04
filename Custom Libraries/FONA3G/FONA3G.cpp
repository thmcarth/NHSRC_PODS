//Fona 3G Libray
//Hawes Collier with quotes from Arduino's FONAtest

#include "FONA3G.h"
#include "Arduino.h"

//CONSTRUCTOR
//FONA3G will use functions that simplify HTTP Requests 
//
//
#include "Adafrut_FONA.h"
#include <SoftwareSerial.h>


// These are default values for now

#define FONA_RX 2
#define FONA_TX 3
#define FONA_RST 4


// Variables needed in program

/*
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);
char replybuffer[255];
*/

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Optionally configure a GPRS APN, username, and password.
  // You might need to do this to access your network's GPRS/data
  // network.  Contact your provider for the exact APN, username,
  // and password values.  Username and password are optional and
  // can be removed, but APN is required.
  //fona.setGPRSNetworkSettings(F("your APN"), F("your username"), F("your password"));

FONA3G::FONA3G(){
	
}
void FONA3G::ReadRSSI(void) {
	uint8_t n = fona.getRSSI();
        int8_t r;

        Serial.print(F("RSSI = ")); Serial.print(n); Serial.print(": ");
        if (n == 0) r = -115;
        if (n == 1) r = -111;
        if (n == 31) r = -52;
        if ((n >= 2) && (n <= 30)) {
          r = map(n, 2, 30, -110, -54);
        }
        Serial.print(r); Serial.println(F(" dBm"));
	
}
void FONA3G::PostRequest(void) {
	 uint16_t statuscode;
        int16_t length;
        char url[80];  //this will be a constant soon enough
        char data[80];

        FONA3G::flushSerial();
        Serial.println(F("NOTE: in beta! Use simple websites to post!"));
        Serial.println(F("URL to post (e.g. httpbin.org/post):"));
        Serial.print(F("http://")); FONA3G::readline(url, 79);
        Serial.println(url);
        Serial.println(F("Data to post (e.g. \"foo\" or \"{\"simple\":\"json\"}\"):"));
        FONA3G::readline(data, 79);
        Serial.println(data);

        Serial.println(F("****"));
        if (!fona.HTTP_POST_start(url, F("text/plain"), (uint8_t *) data, strlen(data), &statuscode, (uint16_t *)&length)) {
          Serial.println("Failed!");
          break;
        }
        while (length > 0) {
          while (fona.available()) {
            char c = fona.read();

#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
            loop_until_bit_is_set(UCSR0A, UDRE0); /* Wait until data register empty. */
            UDR0 = c;
#else
            Serial.write(c);
#endif

            length--;
            if (! length) break;
          }
        }
        Serial.println(F("\n****"));
        fona.HTTP_POST_end();
	
}

void FONA3G::ReadSMS(void) {
	FONA3G::flushSerial();
        Serial.print(F("Read #"));
        uint8_t smsn = readnumber();
        Serial.print(F("\n\rReading SMS #")); Serial.println(smsn);

        // Retrieve SMS sender address/phone number.
        if (! fona.getSMSSender(smsn, replybuffer, 250)) {
          Serial.println("Failed!");
          break;
        }
        Serial.print(F("FROM: ")); Serial.println(replybuffer);

        // Retrieve SMS value.
        uint16_t smslen;
        if (! fona.readSMS(smsn, replybuffer, 250, &smslen)) { // pass in buffer and max len!
          Serial.println("Failed!");
          break;
        }
        Serial.print(F("***** SMS #")); Serial.print(smsn);
        Serial.print(" ("); Serial.print(smslen); Serial.println(F(") bytes *****"));
        Serial.println(replybuffer);
        Serial.println(F("*****"));
	
}

void FONA3G::DeleteSMS(void) {
	// delete an SMS
        FONA3G::flushSerial();
        Serial.print(F("Delete #"));
        uint8_t smsn = FONA3G::readnumber();

        Serial.print(F("\n\rDeleting SMS #")); Serial.println(smsn);
        if (fona.deleteSMS(smsn)) {
          Serial.println(F("OK!"));
        } else {
          Serial.println(F("Couldn't delete"));
        }
      
}

void FONA3G::sendSMS(void) {
	char sendto[21], message[141];
        FONA3G::flushSerial();
        Serial.print(F("Send to #"));
        FONA3G::readline(sendto, 20);
        Serial.println(sendto);
        Serial.print(F("Type out one-line message (140 char): ")); // need to set messages.
        FONA3G::readline(message, 140);
        Serial.println(message);
        if (!fona.sendSMS(sendto, message)) {
          Serial.println(F("Failed"));
        } else {
          Serial.println(F("Sent!"));
        }
	
}

void FONA3G::addInt(char* name,int value, char* units){
	if (dataNum == 0)
{
sprintf(tempData,"%s%s;%i;%s;Green",tempData,name,value,units);
}else
{
sprintf(tempData,"%s;%s;%i;%s;Green",tempData,name,value,units);
}
++dataNum;
}
void FONA3G::addFloat(char* name,int value, char* units){
	if (dataNum == 0)
{
sprintf(tempData,"%s%s;%f;%s;Green",tempData,name,value,units);
}else
{
sprintf(tempData,"%s;%s;%f;%s;Green",tempData,name,value,units);
}
++dataNum;
}
void FONA3G::addStr(char* name,int value, char* units){
	if (dataNum == 0)
{
sprintf(tempData,"%s%s;%s;%s;Green",tempData,name,value,units);
}else
{
sprintf(tempData,"%s;%s;%s;%s;Green",tempData,name,value,units);
}
++dataNum;
}

void FONA3G::ReadWebsiteURL(void) {
	uint16_t statuscode;
        int16_t length;
        char url[80];

        FONA3G::flushSerial();
        Serial.println(F("NOTE: in beta! Use small webpages to read!"));
        Serial.println(F("URL to read (e.g. wifitest.adafruit.com/testwifi/index.html):"));
        Serial.print(F("http://")); FONA3G::readline(url, 79);
        Serial.println(url);

        Serial.println(F("****"));
        if (!fona.HTTP_GET_start(url, &statuscode, (uint16_t *)&length)) {
          Serial.println("Failed!");
          return;
        }
        while (length > 0) {
          while (fona.available()) {
            char c = fona.read();

            // Serial.write is too slow, we'll write directly to Serial register!
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
            loop_until_bit_is_set(UCSR0A, UDRE0); /* Wait until data register empty. */
            UDR0 = c;
#else
            Serial.write(c);
#endif
            length--;
            if (! length) break;
          }
        }
        Serial.println(F("\n****"));
        fona.HTTP_GET_end();
	
}
void FONA3G::TurnOnGPRS(void) {
	  // turn GPRS on
        if (!fona.enableGPRS(true))
          Serial.println(F("Failed to turn on"));
        return;
	
}
void FONA3G::TurnOffGPRS(void) {
	if (!fona.enableGPRS(false))
          Serial.println(F("Failed to turn off"));
	return;
}


void FONA3G::flushSerial(Serial){
	while (Serial.available())
    Serial.read();
}

uint16_t FONA3G::readnumber(){
	uint16_t x = 0;
  char c;
  while (! isdigit(c = FONA3G::readBlocking())) {
    //Serial.print(c);
  }
  Serial.print(c);
  x = c - '0';
  while (isdigit(c = FONA3G::readBlocking())) {
    Serial.print(c);
    x *= 10;
    x += c - '0';
  }
  return x;
}

char FONA3G::readBlocking(){
	
	while (!Serial.available());
  return Serial.read();
  
}

uint8_t FONA3G::readline(char *buff, uint8_t maxbuff, uint16_t timeout) {
  uint16_t buffidx = 0;
  boolean timeoutvalid = true;
  if (timeout == 0) timeoutvalid = false;

  while (true) {
    if (buffidx > maxbuff) {
      //Serial.println(F("SPACE"));
      break;
    }

    while (Serial.available()) {
      char c =  Serial.read();

      //Serial.print(c, HEX); Serial.print("#"); Serial.println(c);

      if (c == '\r') continue;
      if (c == 0xA) {
        if (buffidx == 0)   // the first 0x0A is ignored
          continue;

        timeout = 0;         // the second 0x0A is the end of the line
        timeoutvalid = true;
        break;
      }
      buff[buffidx] = c;
      buffidx++;
    }

    if (timeoutvalid && timeout == 0) {
      //Serial.println(F("TIMEOUT"));
      break;
    }
    delay(1);
  }
  buff[buffidx] = 0;  // null term
  return buffidx;
} 




