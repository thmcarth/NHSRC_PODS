
/*This code was written to work with the 094 sensor pod.
 * //Initial- v0,2 -
 * //v0.3 - Removed all BOSS2 related code until repair trip to Edison.

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
int totalBoss1 = 0;
int totalBoss2 = 0;
int totalBoss3 = 0;
int boss1textCounter = 0;
int boss2textCounter = 0;
int boss3textCounter = 0;
unsigned long oneCommandTimeout = millis();
unsigned long twoCommandTimeout = millis();
unsigned long threeCommandTimeout = millis();
unsigned long sendTimer = -1750000; //timer to sendQuery every (minsToCheck) minutes
unsigned long checkTimer = millis();; //timer to check 5 mins after every query.
bool waiting;
int minsToCheck = 30; //Checks every X minutes
////////////////////////////////////////////////

/////ISCO Options
bool oneGrabDesire = true;
bool oneGrabState = true;
bool twoGrabDesire = true;
bool twoGrabState = true;
bool oneISCODesire = true;
bool oneISCOState = true;
bool twoISCODesire = true;
bool twoISCOState = true;
bool BOSS4Online = true;
///////////////////////ISCO Options


unsigned long textTwoTimer = millis();

//FONA HTTP
#include "Adafruit_FONA.h"
char senderNum[20];    //holds number of last number to text SIM
int smslength = 0;
#define FONA_RST 4
char replybuffer[1000]; //holds Fonas text reply
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
char startDate[30];  // dateTime to be written to SD card header
char dateTimeVIPER[40]; //dateTime for VIPER string
#include <SD.h>
const int chipSelect = BUILTIN_SDCARD; //For Teensy 3.5, SPI for SD card is separate
File myFile;
int minsToSave = 1;
int date;  //holds current date so new file can be written at midnight
unsigned long lastSave = millis();  // holds time since execution of last save to ensure data is saved every 60 seconds
int Unit1Restarts = 0;
int Unit2Restarts = 0;
int Unit3Restarts = 0;

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
  buildStartDateTime();
  date = day();
  Watchdog.reset();
}

void loop() {
  delay(3000);
  if (date != day())
  {
    date = day();
    setfileName();
    writeNum = 0;
    Unit1Restarts = 0;
    Unit2Restarts = 0;
    Unit3Restarts = 0;
  }

  if (millis() - sendTimer > 1000 * 60 * minsToCheck)
  {
    boss1textCounter=0;
    boss2textCounter=0;
    boss3textCounter=0;
    Boss1Status = false;
    Boss2Status = false;
    Boss3Status = false;
    sendTimer = millis();
    if (BOSS4Online)
    {
      sendQuery();
    }
    waiting = true;
    checkTimer = millis();
  } else
  {
    Serial.print("Sending next query in "); Serial.print(((1000 * 60 * minsToCheck) - ((millis() - sendTimer))) / 1000); Serial.println(" seconds");
  }
  if (millis() - textTwoTimer > 180000)
  {
    if (BOSS4Online)
    {

      textTwoTimer = millis();
      Serial.println("Texting BOSS2");
      //sendSMS(Boss2, "0");
      delay(1000);
      //sendSMS(Boss2, "1");
    }
  } else
  {
   // Serial.print("Texting BOSS2 in "); Serial.print(((1000 * 60 * 3) - ((millis() - textTwoTimer))) / 1000); Serial.println(" seconds");
  }

  checkTexts();

  if (boss1textCounter > 15)
  {
    Serial.println("Too many texts from Boss1, sending 1");
    sendSMS(Boss1,"1");
    boss1textCounter = 0;
  }
  if (boss2textCounter > 20)
  {
    Serial.println("Too many texts from Boss2, sending 1");
    //sendSMS(Boss2,"1");
    boss2textCounter = 0;
  }
  if (boss3textCounter > 15)
  {
    Serial.println("Too many texts from Boss3, sending 1");
    sendSMS(Boss3,"1");
    boss3textCounter = 0;
  }
  if (commandNum == 9)
  {
    sendStatus();
    commandNum = -1;
  }
  if (commandNum == 5)
  {
    toggleBoss4();
    commandNum = -1;
  }
  if (commandNum == 1)
  {
    change1Mode();
    sendStatus();
    commandNum = -1;
  }
  if (commandNum == 2)
  {
    change1State();
    sendStatus();
    commandNum = -1;
  }
  if (commandNum == 3)
  {
    change2Mode();
    sendStatus();
    commandNum = -1;
  }
  if (commandNum == 4)
  {
    change2State();
    sendStatus();
    commandNum = -1;
  }

  if (millis() - checkTimer > 300000 && waiting)
  {
    waiting = false;
    if (BOSS4Online)
    {
      processRestarts();
    }
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
  //sendSMS(Boss2, "0");
  //delay(1000);
  //sendSMS(Boss2, "1");
  //delay(1000);
  sendSMS(Boss3, "0");
  delay(1000);
}

void processRestarts() {
  if (!Boss1Status)
  {
    Serial.println("No response from Boss 1. Restarting....");
    restartBoss1();
    Serial.println("Changing to GRAB mode...");
    sendSMS(Boss1, "9");
    ++Unit1Restarts;
  }
  if (!Boss2Status)
  {
 //  Serial.println("No response from Boss 2. Restarting....");
 //   restartBoss2();
 //   Serial.println("Changing to GRAB mode...");
 //   sendSMS(Boss2, "9");
 //   ++Unit2Restarts;
  }
  if (!Boss3Status)
  {
    Serial.println("No response from Boss 3. Restarting....");
    restartBoss3();
    ++Unit3Restarts;
  }
  saveData();
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
    Serial.print("Ringing = "); Serial.println(ringing);
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
    Serial.print("Ringing = "); Serial.println(ringing);
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
    Serial.print("Ringing = "); Serial.println(ringing);
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
      { ++boss1textCounter;
        ++totalBoss1;

        Serial.print("Received reply from Boss1,");Serial.print(totalBoss1);Serial.print(",");Serial.println(boss1textCounter);
        if (BOSS4Online)
        {
          Boss1Status = true;
          //find mode
          delay(500);
          checkMode(1);
          delay(500);
          checkISCO(1);
          //find Isco state
        }
      }
      if (strcmp(senderNum, Boss2) == 0)
      {
       // ++boss2textCounter;
       // ++totalBoss2;
       // Serial.print("Received reply from Boss2,");Serial.print(totalBoss2);Serial.print(",");Serial.println(boss2textCounter);
       // if (BOSS4Online)
       // {
       //  Boss2Status = true;
       //   delay(500);
       //   checkMode(2);
       //   delay(500);
       //   checkISCO(2);
       // }
      }
      if (strcmp(senderNum, Boss3) == 0)
      {
        if (BOSS4Online)
        {
          Boss3Status = true;
        }
        ++boss3textCounter;
        ++totalBoss3;
       Serial.print("Received reply from Boss3,");Serial.print(totalBoss3);Serial.print(",");Serial.println(boss3textCounter);
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

void checkMode(int boss)
{
  if (boss == 1)
  {
    int modePos = 0;
    char mode[10];
    sprintf(mode, "%s", "");
    for (int x = 0 ; x < 500 ; ++x)
    {
      if (replybuffer[x] == 't' && replybuffer[x + 1] == 'o') //Found mode
      {
        modePos = x + 3;
        Serial.print("Found modePos at "); Serial.println(modePos);
        x = 600;
      }


    }
    Serial.print("Next 4..."); Serial.print(replybuffer[modePos]); Serial.print(replybuffer[modePos + 1]); Serial.print(replybuffer[modePos + 2]); Serial.println(replybuffer[modePos + 3]);


    if (modePos > 50 && smslength > 90)
    {
      if (replybuffer[modePos] == 'A' && replybuffer[modePos + 1] == 'U') //Currently in grab
      {
        oneGrabState = true;
        Serial.println("Currently in Grab mode");
        if (!oneGrabDesire)
        {
          Serial.println("Auto mode Desired. Switching...");
          if (millis() - oneCommandTimeout > 60000)
          {
            sendSMS(Boss1, "9");
            oneCommandTimeout = millis();
          } else
          {
            Serial.println("Too soon. Waiting for previous command to take effect.");
          }
        } else
        {
          Serial.println("OK");
        }
      } else
      {
        if (replybuffer[modePos] == 'G' && replybuffer[modePos + 1] == 'R') //Currently in auto
        {
          oneGrabState = false;
          Serial.println("Currently in Auto mode");
          if (oneGrabDesire)
          {
            if (millis() - oneCommandTimeout > 60000)
            {
              Serial.println("Grab mode Desired. Switching...");
              sendSMS(Boss1, "9");
              oneCommandTimeout = millis();
            } else
            {
              Serial.println("Too soon. Waiting for previous command to take effect.");
            }
          }
          else
          {
            Serial.println("OK");
          }
        } else
        {
          Serial.println("Error parsing mode state");
        }
      }
    }

  }

  if (boss == 2)
  {
    int modePos = 0;
    char mode[10];
    sprintf(mode, "%s", "");
    for (int x = 0 ; x < 500 ; ++x)
    {
      if (replybuffer[x] == 't' && replybuffer[x + 1] == 'o') //Found mode
      {
        modePos = x + 3;
        Serial.print("Found modePos at "); Serial.println(modePos);
        x = 600;
      }


    }
    Serial.print("Next 4..."); Serial.print(replybuffer[modePos]); Serial.print(replybuffer[modePos + 1]); Serial.print(replybuffer[modePos + 2]); Serial.println(replybuffer[modePos + 3]);


    if (modePos > 50 && smslength > 90)
    {
      if (replybuffer[modePos] == 'A' && replybuffer[modePos + 1] == 'U') //Currently in grab
      {
        twoGrabState = true;
        Serial.println("Currently in Grab mode");
        if (!twoGrabDesire)
        {
          if (millis() - twoCommandTimeout > 60000)
          {
            Serial.println("Auto mode Desired. Switching...");
            sendSMS(Boss2, "9");
            twoCommandTimeout = millis();
          } else
          {
            Serial.println("Too soon. Waiting for previous command to take effect.");
          }
        } else
        {
          Serial.println("OK");
        }
      } else
      {
        if (replybuffer[modePos] == 'G' && replybuffer[modePos + 1] == 'R') //Currently in auto
        {
          twoGrabState = false;
          Serial.println("Currently in Auto mode");
          if (twoGrabDesire)
          {
            if (millis() - twoCommandTimeout > 60000)
            {
              Serial.println("Grab mode Desired. Switching...");
              sendSMS(Boss2, "9");
              twoCommandTimeout = millis();
            } else
            {
              Serial.println("Too soon. Waiting for previous command to take effect.");
            }
          }
          else
          {
            Serial.println("OK");
          }
        } else
        {
          Serial.println("Error parsing mode state");
        }
      }
    }

  }
}

void checkISCO(int boss)
{
  if (boss == 1)
  {
    int statePos = 0;
    //char mode[10];
    //sprintf(mode, "%s", "");
    for (int x = 0 ; x < 500 ; ++x)
    {
      if (replybuffer[x] == 'r' && replybuffer[x + 1] == 'n') //Found state
      {
        statePos = x + 3;
        Serial.print("Found statePos at "); Serial.println(statePos);
        x = 600;
      }


    }
    Serial.print("Next 3..."); Serial.print(replybuffer[statePos]); Serial.print(replybuffer[statePos + 1]); Serial.println(replybuffer[statePos + 2]);

    if (statePos > 50 && smslength > 90)
    {
      if (replybuffer[statePos] == 'o' && replybuffer[statePos + 1] == 'f') //Currently on
      {
        oneISCOState = true;
        Serial.println("Currently On");
        if (!oneISCODesire)
        {
          if (millis() - oneCommandTimeout > 60000)
          {
            Serial.println("Off Desired. Switching...");
            sendSMS(Boss1, "8");
            oneCommandTimeout = millis();
          } else
          {
            Serial.println("Too soon. Waiting for previous command to take effect.");
          }
        } else
        {
          Serial.println("OK");
        }
      } else
      {
        if (replybuffer[statePos] == 'o' && replybuffer[statePos + 1] == 'n') //Currently off
        {
          oneISCOState = false;
          Serial.println("Currently Off");
          if (oneISCODesire)
          {
            if (millis() - oneCommandTimeout > 60000)
            {
              Serial.println("On Desired. Switching...");
              sendSMS(Boss1, "8");
              oneCommandTimeout = millis();
            } else
            {
              Serial.println("Too soon. Waiting for previous command to take effect.");
            }
          }
          else
          {
            Serial.println("OK");
          }
        } else
        {
          Serial.println("Error parsing ISCO state");
        }
      }
    }

  }

  if (boss == 2)
  {
    int statePos = 0;
    //char mode[10];
    //sprintf(mode, "%s", "");
    for (int x = 0 ; x < 500 ; ++x)
    {
      if (replybuffer[x] == 'r' && replybuffer[x + 1] == 'n') //Found state
      {
        statePos = x + 3;
        Serial.print("Found statePos at "); Serial.println(statePos);
        x = 600;
      }


    }
    Serial.print("Next 3..."); Serial.print(replybuffer[statePos]); Serial.print(replybuffer[statePos + 1]); Serial.println(replybuffer[statePos + 2]);

    if (statePos > 50 && smslength > 90)
    {
      if (replybuffer[statePos] == 'o' && replybuffer[statePos + 1] == 'f') //Currently on
      {
        twoISCOState = true;
        Serial.println("Currently On");
        if (!twoISCODesire)
        {
          if (millis() - twoCommandTimeout > 60000)
          {
            Serial.println("Off Desired. Switching...");
            sendSMS(Boss2, "8");
            twoCommandTimeout = millis();
          } else
          {
            Serial.println("Too soon. Waiting for previous command to take effect.");
          }
        } else
        {
          Serial.println("OK");
        }
      } else
      {
        if (replybuffer[statePos] == 'o' && replybuffer[statePos + 1] == 'n') //Currently off
        {
          twoISCOState = false;
          Serial.println("Currently Off");
          if (twoISCODesire)
          {
            if (millis() - twoCommandTimeout > 60000)
            {
              Serial.println("On Desired. Switching...");
              sendSMS(Boss2, "8");
              twoCommandTimeout = millis();
            } else
            {
              Serial.println("Too soon. Waiting for previous command to take effect.");
            }
          }
          else
          {
            Serial.println("OK");
          }
        } else
        {
          Serial.println("Error parsing ISCO state");
        }
      }
    }

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
  sprintf(replybuffer, "%s", "");
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
  smslength = smslen;
  Serial.print(F("***** SMS #")); Serial.print(number);
  Serial.print(" ("); Serial.print(smslen); Serial.println(F(") bytes *****"));
  Serial.println(replybuffer);
  commandNum = atoi(replybuffer);
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

      myFile.println("TIMESTAMP(UTC),Status,1Restarts,2Restarts,3Restarts");
    }
    delay(10);
    buildDateTime();
    myFile.print(dateTime);
    myFile.print(",");
    if (Boss1Status && Boss2Status)
    {
      if (Boss3Status)
      {
        myFile.print("No restarts needed");
      } else
      {
        myFile.print("Restarted Unit 3");
      }
    } else
    {
      myFile.print("Restarted ");
      if (!Boss1Status)
      {
        myFile.print("Unit 1");
      }
      if (!Boss2Status)
      {
        myFile.print("Unit 2");
      }
      if (!Boss3Status)
      {
        myFile.print("Unit 3");
      }
    }
    myFile.print(",");
    myFile.print(Unit1Restarts);
    myFile.print(",");
    myFile.print(Unit2Restarts);
    myFile.print(",");
    myFile.println(Unit3Restarts);



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


void buildStartDateTime()
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
  sprintf(startDate, "%i%s%s%s%s%s%s%s%s%s%s", year(), "-", monthChar, "-", dateChar, "T", hourChar, ":", minuteChar, ":", secondChar);
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

void sendStatus()
{
  Serial.println("Sending Status Message");
  char message[500];
  sprintf(message, "%s", "Pod 1,");
  if (oneGrabDesire)
  {
    sprintf(message, "%s%s", message, "Grab,");
  } else
  {
    sprintf(message, "%s%s", message, "Auto,");
  }
  if (oneISCODesire)
  {
    sprintf(message, "%s%s", message, "On,");
  } else
  {
    sprintf(message, "%s%s", message, "Off,");
  }
  sprintf(message, "%s%s", message, "\n");
  sprintf(message, "%s%s", message, "Pod 2,");
  if (twoGrabDesire)
  {
    sprintf(message, "%s%s", message, "Grab,");
  } else
  {
    sprintf(message, "%s%s", message, "Auto,");
  }
  if (twoISCODesire)
  {
    sprintf(message, "%s%s", message, "On,");
  } else
  {
    sprintf(message, "%s%s", message, "Off,");
  }
  sprintf(message, "%s%s", message, "\n");
  sprintf(message, "%s%s", message, "1=1 Mode");
  sprintf(message, "%s%s", message, "\n");
  sprintf(message, "%s%s", message, "2=1 State");
  sprintf(message, "%s%s", message, "\n");
  sprintf(message, "%s%s", message, "3=2 Mode");
  sprintf(message, "%s%s", message, "\n");
  sprintf(message, "%s%s", message, "4=2 State");
  sprintf(message, "%s%s", message, "\n");
  sprintf(message, "%s%s", message, "5=");
  if (BOSS4Online)
  {
    sprintf(message, "%s%s", message, "Disable");
  } else
  {
    sprintf(message, "%s%s", message, "Enable");
  }
  sprintf(message, "%s%s%s", message, " BOSS 4","\n");
  sprintf(message,"%s%s%i%s",message, "texts1,",totalBoss1,"\n");
  sprintf(message,"%s%s%i%s",message, "texts2,",totalBoss2,"\n");
  sprintf(message,"%s%s%i%s",message, "texts3,",totalBoss3,"\n");
  sprintf(message, "%s%s%s", message, "since ",startDate);
  sendSMS(senderNum, message);
}

void change1Mode()
{
  if (oneGrabDesire)
  {
    oneGrabDesire = false;
  } else
  {
    oneGrabDesire = true;
  }
}
void toggleBoss4()
{
  if (BOSS4Online)
  {
    Serial.println("Turning BOSS4 Off");
    BOSS4Online = false;
    sendSMS(senderNum, "Boss 4 Off");
  } else
  {
    Serial.println("Turning BOSS4 On");
    BOSS4Online = true;
    sendSMS(senderNum, "Boss 4 On");
  }
}
void change1State()
{
  if (oneISCODesire)
  {
    oneISCODesire = false;
  } else
  {
    oneISCODesire = true;
  }
}
void change2Mode()
{
  if (twoGrabDesire)
  {
    twoGrabDesire = false;
  } else
  {
    twoGrabDesire = true;
  }
}
void change2State()
{
  if (twoISCODesire)
  {
    twoISCODesire = false;
  } else
  {
    twoISCODesire = true;
  }
}

