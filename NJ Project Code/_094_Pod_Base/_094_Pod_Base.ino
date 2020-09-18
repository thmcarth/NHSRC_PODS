
/*This code was written to work with the 094 sensor pod.

  Tim McArthur
*/
int unit = 4;

//Char arrays to hold time information for posting to VIPER

char monthChar[5];
char dateChar[5];
char hourChar[5];
char minuteChar[5];
char secondChar[5];

/////////////////////////////////////This section is taken from the Ubidots library which was modified to work with VIPER. Could change it since the TOKEN is irrelevant. But if it ain't broke....
#include <Ubidots_FONA.h>
#define TOKEN "A1E-ft1AUBxmYrb1T5Rm37dnAHU7pjyXEC"  // Replace it with your Ubidots token
#define APN "wholesale" // Assign the APN 
#define USER ""  // If your apn doesnt have username just put ""
#define PASS ""  // If your apn doesnt have password just put ""
Ubidots client(TOKEN);
///////////////////////////////////////////////end weird Ubidots section



//////////////////////////////////

#include <Time.h>
//Watchdog
#include <Adafruit_SleepyDog.h>

//////////////////////////////////////////////

//Static numbers to text to test pods
char* Boss1 = "+19199492342";
char* Boss2 = "+19196383272";
char* Boss3 = "+19196389975";
bool Boss1Status = false;
bool Boss2Status = false;
bool Boss3Status = false;
unsigned long sendTimer = -250000; //timer to sendQuery every (minsToCheck) minutes
unsigned long checkTimer = millis();; //timer to check 5 mins after every query.
bool waiting;
int minsToCheck = 10; //Checks every 10 minutes
////////////////////////////////////////////////

//FONA HTTP


#include "Adafruit_FONA.h"
char senderNum[20];    //holds number of last number to text SIM
#define FONA_RST 4
char replybuffer[255]; //holds Fonas text reply
HardwareSerial *fonaSerial = &Serial5;
HardwareSerial *GPRSSerial = &Serial5;
#define FONA_PKEY 37 //Power Key pin to turn on/off Fona
#define FONA_PS 38 //Power Status PIN
#define FONA_RI_BYPASS 35 //Pull this pin high to bypass the ring indicator pin when resetting the Fona from the Teensy so the Fona doesn't lock up
bool fonaPower = true; //Whether or not Fona is currently powered on
int fonaStatus;
int success = 0;
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);
/////FONA//

uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);
int commandNum = -1; //integer value for which text command has been sent
uint8_t type;
unsigned long deleteTextsTimer = millis(); //after a set timeout, all(30) texts are deleted from SIM
bool _debug = true;

//SD
int writeNum; // integer to control whether or not the header data is written to SD file
char fileName[20];  //fileName built from date
char dateTime[30];  // dateTime to be written to SD card header
char dateTimeVIPER[40]; //dateTime for VIPER string
#include <SD.h>
const int chipSelect = BUILTIN_SDCARD; //For Teensy 3.5, SPI for SD card is separate
File myFile;
int minsToSave = 1;
int date;  //holds current date so new file can be written at midnight
unsigned long lastSave = millis();  // holds time since execution of last save to ensure data is saved every 60 seconds
///////////////SD



void setup() {
  Serial.begin(9600);
  delay(2000);
  int countdownMS = Watchdog.enable(300000);
  Serial.print("Enabled the watchdog with max countdown of ");
  Serial.print(countdownMS, DEC);
  Serial.println(" milliseconds!");
  Serial.println();
  delay(2000);
  setSyncProvider(getTeensy3Time);
  Serial.print("Initializing SD card...");  //startup SD card

  if (!SD.begin(chipSelect)) {
    Serial.println("initialization failed!");
  }
  Serial.println("initialization done.");

  pinMode(FONA_PKEY, OUTPUT); //Power Key INPUT
  pinMode(FONA_RI_BYPASS, OUTPUT); //FONA RI BYPASS signal
  pinMode(FONA_PS, INPUT); //Power Status INPUT
  //pinMode(levelPin, INPUT);
  delay(50);
  digitalWrite(FONA_RI_BYPASS, HIGH);
  Watchdog.reset();
  delay(5000);
  Serial.print("Fona is...");
  int fonaStatus = getFonaStatus();
  Serial.print(fonaStatus);
  if (fonaStatus > 500)
  {
    Serial.println(" On!");
    digitalWrite(FONA_RI_BYPASS, LOW);
  } else
  {
    Serial.println(" off.");
  }
  if (fonaStatus < 500)
  {
    digitalWrite(FONA_RI_BYPASS, HIGH);
    Serial.println("Turning on Fona");
    digitalWrite(FONA_PKEY, HIGH);
    delay(2000);
    digitalWrite(FONA_PKEY, LOW); //turn on Fona
    delay(5000);
    digitalWrite(FONA_PKEY, HIGH); //turn on Fona
    delay(2000);
    digitalWrite(FONA_RI_BYPASS, LOW);
    Serial.println("Done");

  }
  Watchdog.reset();

  GPRSSerial->begin(19200);
  if (! client.init(*GPRSSerial)) {
    Serial.println(F("Couldn't find FONA"));
    //while (1);
  }

  //fonaSerial->begin(19200);
  if (! fona.begin(*fonaSerial)) {
    Serial.println(F("Couldn't find FONA"));
    //while (1);
  }
  type = fona.type();
  Serial.println(F("FONA is OK"));
  Serial.print(F("Found "));
  switch (type) {
    case FONA800L:
      Serial.println(F("FONA 800L")); break;
    case FONA800H:
      Serial.println(F("FONA 800H")); break;
    case FONA808_V1:
      Serial.println(F("FONA 808 (v1)")); break;
    case FONA808_V2:
      Serial.println(F("FONA 808 (v2)")); break;
    case FONA3G_A:
      Serial.println(F("FONA 3G (American)")); break;
    case FONA3G_E:
      Serial.println(F("FONA 3G (European)")); break;
    default:
      Serial.println(F("???")); break;

      //      client.setApn(APN, USER, PASS);
  }

  Watchdog.reset();
}

void loop() {
  delay(3000);

  if (millis() - sendTimer > 1000 * 60 * minsToCheck)
  {
    Boss1Status = false;
    Boss2Status = false;
    Boss3Status = false;
    sendTimer = millis();
    sendQuery();
    waiting = true;
    checkTimer = millis();
  } else
  {
    Serial.print("Sending next query in "); Serial.print(((1000 * 60 * minsToCheck) - ((millis() - sendTimer))) / 1000); Serial.println(" seconds");
  }

  checkTexts();

  if (millis() - checkTimer > 300000 && waiting)
  {
    waiting = false;
    processRestarts();
    checkTimer = millis();
  } else
  {
    if (waiting)
    {
      Serial.print("Waiting to process Restarts...."); Serial.print(((300000) - ((millis() - checkTimer))) / 1000); Serial.println(" seconds");
    } else
    {
      Serial.print("Waiting to process Restarts after next query....");
       if (millis() - deleteTextsTimer > 300000)
      {
        for (int y = 0 ; y < 30 ; ++y)
        {
          deleteSMS(y);
        }
        deleteTextsTimer = millis();
      }
    }
  }

  Watchdog.reset();

  if (millis() - lastSave > minsToSave * 60000) //Save data every (minsToSave) minutes
  {
    Serial.println("----------");
    Serial.println("SAVING");
    Serial.println("----------");
    Serial.println("SAVING");
    Serial.println("----------");
    Serial.println("SAVING");
    saveData();
    lastSave = millis();
  }

  Serial.print("Fona onCount = "); Serial.println(getFonaStatus());

  if (getFonaStatus() < 500) //If Fona turns off for some reason,turn it on
  {
    toggleFona();
  }



  for (int x = 0; x < 64 ; ++x)
  {
    char a = Serial.read();
    char b = fonaSerial->read();
  }


}

void sendQuery() {
  Serial.println("Sending texts to all Boss Units");
  sendSMS(Boss1, "0");
  delay(1000);
  sendSMS(Boss2, "0");
  delay(1000);
  sendSMS(Boss3, "0");
  delay(1000);
}

void processRestarts() {
  if (!Boss1Status)
  {
    Serial.println("No response from Boss 1. Restarting....");
    restartBoss1();
  }
  if (!Boss2Status)
  {
    Serial.println("No response from Boss 2. Restarting....");
    restartBoss2();
    Serial.println("Turning off Boss 2 ISCO...");
    sendSMS(Boss2,"9");
  }
  if (!Boss3Status)
  {
    Serial.println("No response from Boss 3. Restarting....");
    restartBoss3();
  }
  Boss1Status = false;
  Boss2Status = false;
  Boss3Status = false;
}

bool restartBoss1()
{
  fona.callPhone(Boss1);
  Serial.println("Dialing Boss 1.");
  delay(10000);
  int ringing = fona.getCallStatus();
  if (ringing > 2)
  {
    Serial.println("...Ringing");
    fona.hangUp();
    return true;
  } else
  {
    Serial.println("There was a problem with the call.");
    Serial.print("Ringing = ");Serial.println(ringing);
    fona.hangUp();
    return false;
  }

}

bool restartBoss2()
{
  fona.callPhone(Boss2);
  Serial.println("Dialing Boss 2.");
  delay(10000);
  int ringing = fona.getCallStatus();
  if (ringing > 2)
  {
    Serial.println("...Ringing");
    fona.hangUp();
    return true;
  } else
  {
    Serial.println("There was a problem with the call.");
    Serial.print("Ringing = ");Serial.println(ringing);
    fona.hangUp();
    return false;
  }

}

bool restartBoss3()
{
  fona.callPhone(Boss3);
  Serial.println("Dialing Boss 3.");
  delay(10000);
  int ringing = fona.getCallStatus();
  if (ringing > 2)
  {
    Serial.println("...Ringing");
    Serial.print("Ringing = ");Serial.println(ringing);
    fona.hangUp();
    return true;
  } else
  {
    Serial.println("There was a problem with the call.");
    fona.hangUp();
    return false;
  }

}

void checkTexts()
{

  int8_t numSMS = readSMSNum();
  Serial.print("readSMSNum()="); Serial.println(numSMS);
  if (numSMS > 0)
  {
    Serial.println("Received text Message!");
    //Serial.print("You have ");Serial.print(numSMS);Serial.println(" text messages");
    for (int x = 0 ; x < numSMS ; ++x)
    {
      int tries = 0;
      do {
        ++tries;
        success = readSMS(x);
        ++x;
        Serial.print("Success ="); Serial.println(success);
      } while (success == 0  && tries < 30);


      success = deleteSMS(x - 1);

     
      if (strcmp(senderNum, Boss1) == 0)
      {
        Boss1Status = true;
        Serial.println("Received reply from Boss1");
      }
      if (strcmp(senderNum, Boss2) == 0)
      {
        Boss2Status = true;
        Serial.println("Received reply from Boss2");
      }
      if (strcmp(senderNum, Boss3) == 0)
      {
        Boss3Status = true;
        Serial.println("Received reply from Boss3");
      }
    }

  }
  else
  {
    Serial.println("No New SMS");
    Serial.println("");
    Serial.println("");
  }
}




///////////////////////////////***********************FONA FUNCTIONS

/*** SMS ***/
int readSMSNum() {
  // read the number of SMS's!
  int smsnum;
  char response[50];
  flushFonaSerial();
  //Serial.println("Writing AT Command to Serial 1");
  fonaSerial->print("AT+CPMS?");
  fonaSerial->write(13);
  delay(50);
  while (fonaSerial->available())
  {
    delay(50);
    for (int x = 0; x < fonaSerial->available() ; ++x)
    {
      char a = fonaSerial->read();
      response[x] = a;
    }
  }
  Serial.print("Response from Fona -"); Serial.println(response);
  Serial.print("11th char is "); Serial.println(response[16]);
  smsnum = response[14] - '0';
  //Serial.print("to an int ");Serial.println(atoi(response[10]);
  Serial.print("You have "); Serial.print(smsnum); Serial.println( "messages");
  sprintf(response, "%s", "");
  return smsnum;


}

int readSMS(int8_t number) {
  // read an SMS
  //Serial.print(F("Read #"));
  Serial.print(F("\n\rReading SMS #")); Serial.println(number);

  // Retrieve SMS sender address/phone number.
  if (! fona.getSMSSender(number, replybuffer, 250)) {
    Serial.println("Failed!");
    return 0;
  }
  Serial.print(F("FROM: ")); Serial.println(replybuffer);

  int y = sprintf(senderNum, "%s", replybuffer);
  Serial.print("Sender is ");
  Serial.println(senderNum);

  // Retrieve SMS value.
  uint16_t smslen;
  if (! fona.readSMS(number, replybuffer, 250, &smslen)) { // pass in buffer and max len!
    Serial.println("Failed!");
  }
  Serial.print(F("***** SMS #")); Serial.print(number);
  Serial.print(" ("); Serial.print(smslen); Serial.println(F(") bytes *****"));
  Serial.println(replybuffer);
  //commandNum = atoi(replybuffer);
  Serial.println(F("*****"));
  return 1;
}

int deleteSMS(int8_t number) {
  // delete an SMS
  //Serial.print(F("Delete #"));
  Serial.print(F("\n\rDeleting SMS #")); Serial.println(number);
  if (fona.deleteSMS(number)) {
    Serial.println(F("OK!"));
    return 1;
  } else {
    Serial.println(F("Couldn't delete"));
    return 0;
  }

}

bool sendSMS(char* sendto, char*message) {
  // send an SMS!
  Serial.print(F("Send to #"));
  Serial.println(sendto);
  Serial.print(F("Message (140 char): "));
  Serial.println(message);
  if (!fona.sendSMS(sendto, message)) {
    Serial.println(F("Failed"));
    return false;
  } else {
    Serial.println(F("Sent!"));
    return true;
  }


}

uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout) {
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

bool sendFona(char* message)
{
  char a;
  flushFonaSerial();
  fonaSerial->println(message);
  while (fonaSerial->available())
    a = fonaSerial->read();
  if (a == '0' || a == false)
  {
    return false;
  }
  if (a == '1' || a == true)
  {
    return true;
  }

}

void flushFonaSerial() {
  while (fonaSerial->available())
    fonaSerial->read();
}

///////////////////////////////////////**********************FONA FUNCTIONS


int getFonaStatus()
{

  int onCount = 0;
  int z = 0;

  for (z; z < 1000; ++z)
  {
    if (analogRead(FONA_PS) > 500)
    {
      ++onCount;
    }
    delay(1);
  }
  return onCount;
}

void toggleFona() {
  digitalWrite(FONA_RI_BYPASS, HIGH);
  if (fonaPower)
  {
    digitalWrite(FONA_PKEY, LOW); //turn off Fona
    delay(5000);
    digitalWrite(FONA_PKEY, HIGH); //turn off Fona
    fonaPower = false;
    Serial.println("Turned off Fona");
  } else
  {
    digitalWrite(FONA_PKEY, LOW); //turn on Fona
    delay(5000);
    digitalWrite(FONA_PKEY, HIGH); //turn on Fona
    fonaPower = true;
    Serial.println("Turned on Fona");
  }

}


void printDigits(int digits) {
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}


void digitalClockDisplay() {
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(month());
  Serial.print(" ");
  Serial.print(year());
  Serial.println();
}

void setfileName()
{
  int y = sprintf(fileName, "%s%s%i%s", monthChar, dateChar, year(), ".txt");
  Serial.print("Filename is: "); Serial.println(fileName);
}


void saveData() {

  if (writeNum == 0)
  {
    buildDateTime();
    setfileName();
  }

  myFile = SD.open(fileName, FILE_WRITE);
  // open the file for write at end like the Native SD library
  if (!myFile) {
    Serial.println("opening sd file for write failed");
  } else
  {

    Serial.println("File opened");

    // if the file opened okay, write to it:
    Serial.print("Writing to "); Serial.println(fileName);
    Serial.print("writeNum = "); Serial.println(writeNum);
    if (writeNum == 0)
    {

      writeNum++;
      Serial.println("Writing header");   //Write headers
      Serial.println("");

      myFile.print("## BOSS Unit ");
      myFile.print(unit);
      myFile.println(" ## ");
      myFile.println("");
      //int localHours = hour(local);
      //int utcHours = hour(utc);

      myFile.println("TIMESTAMP(UTC)");
    }
    delay(10);
    buildDateTime();
    // close the file:
    myFile.close();
    Serial.println("done.");
  }


}


void buildDateTime()
{


  if (month() < 10)
  {
    int y = sprintf(monthChar, "%c%i", '0', month());
  } else
  {
    int y = sprintf(monthChar, "%i", month());
  }

  if (day() < 10)
  {
    int y = sprintf(dateChar, "%c%i", '0', day());
  } else
  {
    int y = sprintf(dateChar, "%i", day());
  }

  if (hour() < 10)
  {
    int y = sprintf(hourChar, "%c%i", '0', hour());
  } else
  {
    int y = sprintf(hourChar, "%i", hour());
  }

  if (minute() < 10)
  {
    int y = sprintf(minuteChar, "%c%i", '0', minute());
  } else
  {
    int y = sprintf(minuteChar, "%i", minute());
  }

  if (second() < 10)
  {
    int y = sprintf(secondChar, "%c%i", '0', second());
  } else
  {
    int y = sprintf(secondChar, "%i", second());
  }


  //int y = sprintf(dateTimeVIPER, "%i%s%s%s%s%s%s%s%s%s%s%s%i%s", year(localNow), "-", localMonthChar, "-", localDateChar, "T", localHourChar, ":", minuteChar, ":", secondChar, "-0",tzOffset,":00");
  //Serial.println("In build");
  //Serial.print("DTV -- "); Serial.println(dateTimeVIPER);
  sprintf(dateTime, "%i%s%s%s%s%s%s%s%s%s%s", year(), "-", monthChar, "-", dateChar, "T", hourChar, ":", minuteChar, ":", secondChar);
}

time_t getTeensy3Time()
{
  return Teensy3Clock.get();
}

/*  code to process time sync messages from the serial port   */
#define TIME_HEADER  "T"   // Header tag for serial time sync message

unsigned long processSyncMessage() {
  unsigned long pctime = 0L;
  const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013

  if (Serial.find(TIME_HEADER)) {
    pctime = Serial.parseInt();
    return pctime;
    if ( pctime < DEFAULT_TIME) { // check the value is a valid time (greater than Jan 1 2013)
      pctime = 0L; // return 0 to indicate that the time is not valid
    }
  }
  return pctime;
}


