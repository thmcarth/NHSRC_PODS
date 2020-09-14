
/* * Teensy 3.5 Custom PCB developed to communicate with 3 HydraProbe sensors and stream data to VIPER
   Unit is controlled via SMS commands
   Moisture, temp, dielectric potential and conductivity are saved locally
   Temp,moisture and potential are streamed to VIPER using HTTP Post from a FONA 808 cellular module


  Tim McArthur

*/
//094 Pod code Version 0.1 - Cleaned up working code and added watchdog to testing unit. 2/21/18 - Tim McArthur


//GPS and unit values iused in VIPER Post

char GPS[] = "40.508185 ,-74.357935 0"; //needs a (space 0) at the end, example ="35.068471,-89.944730 0"
int unit = 3;

//////////////////////////////////


//Watchdog
#include <Adafruit_SleepyDog.h>
#include <Time.h>
//////////////////////////////////////////////

//Char arrays to hold time information for posting to VIPER

char monthChar[5];
char dateChar[5];
char hourChar[5];
char minuteChar[5];
char secondChar[5];

///////////////////////////////////////

//Char arrays to hold measurement values for posting to VIPER
char Temp1Char[10];
char Temp2Char[10];
char Temp3Char[10];
char Conductivity1Char[10];
char Conductivity2Char[10];
char Conductivity3Char[10];
char Permittivity1Char[10];
char Permittivity2Char[10];
char Permittivity3Char[10];
char Moisture1Char[10];
char Moisture2Char[10];
char Moisture3Char[10];
char bVChar[10];

///////////////////////////////////////

//Static numbers to text when bottles are full
char* timsPhone = "+19199204318";
//Numbers to accept SMS commands from:
char* annesPhone1 = "+19198860812";
char* annesPhone2 = "+15179451531";
char* katherinesPhone = "+16157148918";
char* worthsPhone = "+19194233837";
char* googlePhone = "+19192301472";
bool success1 = false;
bool success2 = false;
int success = 0;
////////////////////////////////////////////////

//FONA HTTP

/* Shouldn't need any Ubidots Stuff...
  #define APN "wholesale" // Assign the APN
  #define USER ""  // If your apn doesnt have username just put ""
  #define PASS ""  // If your apn doesnt have password just put ""
  #define TOKEN "A1E-ft1AUBxmYrb1T5Rm37dnAHU7pjyXEC"  // Replace it with your Ubidots token
  #define VARIABLE_LABEL_1 "temperature" // Assign the variable label
  #define VARIABLE_LABEL_2 "humidity" // Assign the variable label
  #define VARIABLE_LABEL_3 "pressure" // Assign the variable label
  Ubidots client(TOKEN);
*/
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
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);
int commandNum = -1; //integer value for which text command has been sent
uint8_t type;
unsigned long deleteTextsTimer = millis(); //after a set timeout, all(30) texts are deleted from SIM
bool _debug = true;
unsigned long lastPost = millis();
int minsToPost = 1;
/////////////////////////////////////This section is taken from the Ubidots library which was modified to work with VIPER. Could change it since the TOKEN is irrelevant. But if it ain't broke....
#include <Ubidots_FONA.h>
#define TOKEN "A1E-ft1AUBxmYrb1T5Rm37dnAHU7pjyXEC"  // Replace it with your Ubidots token
#define APN "wholesale" // Assign the APN 
#define USER ""  // If your apn doesnt have username just put ""
#define PASS ""  // If your apn doesnt have password just put ""
Ubidots client(TOKEN);
///////////////////////////////////////////////end weird Ubidots section


////////////////////////////////////

//HydraProbe
#include <HydraProbe.h>

float temp, moisture, conductivity,permittivity;
float temp2, moisture2, conductivity2,permittivity2;
float temp3, moisture3, conductivity3,permittivity3;
HydraProbe moistureSensor;  //define data Pin in Header (library .h file)
HydraProbe moistureSensor2;
HydraProbe moistureSensor3;
bool HPRail = true;
///////////////////////////////////////

//Voltage Readings
#define BattRail_VIN A1
float BattVoltage;
#define FiveRail_VIN A9
float FiveVoltage;
//#define ThreeRail_VIN A5
#define levelPin A20
//float ThreeVoltage;
///////////////////////

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


///////////////////////////RELAYS
bool RQ30 = true;
#define HPRelay 27
#define RQ30Relay 26
unsigned long fonaTimer = millis();
int resetFona = 60; //reset Fona every X minutes
//////////////////////////RELAYS


void setup() {
  Serial.begin(9600);
  int countdownMS = Watchdog.enable(300000);
  Serial.print("Enabled the watchdog with max countdown of ");
  Serial.print(countdownMS, DEC);
  Serial.println(" milliseconds!");
  Serial.println();
  delay(2000);
  //setSyncProvider(getTeensy3Time);
  Serial.print("Initializing SD card...");  //startup SD card

  if (!SD.begin(chipSelect)) {
    Serial.println("initialization failed!");
  }
  Serial.println("initialization done.");

  pinMode(HPRelay, OUTPUT); //Set up HydraProbe relay
  pinMode(FONA_PKEY, OUTPUT); //Power Key INPUT
  pinMode(FONA_RI_BYPASS, OUTPUT); //FONA RI BYPASS signal
  pinMode(FONA_PS, INPUT); //Power Status INPUT
  //pinMode(levelPin, INPUT);
  delay(50);
  digitalWrite(FONA_RI_BYPASS, HIGH);
  pinMode(BattRail_VIN, INPUT);
  digitalWrite(HPRelay, HIGH); //turn it on
  moistureSensor.debugOff(); //Can turn debug on or off to see verbose output (OR NOT)
  moistureSensor.begin(1);
  moistureSensor2.debugOff(); //Can turn debug on or off to see verbose output (OR NOT)
  moistureSensor2.begin(2);
  moistureSensor3.debugOff(); //Can turn debug on or off to see verbose output (OR NOT)
  moistureSensor3.begin(3);
  Watchdog.reset();
  Serial.print("Fona is...");
  Serial.print(getFonaStatus());
  if (getFonaStatus() > 500)
  {
    Serial.println(" On!");
    digitalWrite(FONA_RI_BYPASS, LOW);
  } else
  {
    Serial.println(" off.");
  }
  if (getFonaStatus() < 500)
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

      client.setApn(APN, USER, PASS);
  }
   Serial.begin(9600);
   Watchdog.reset();
}

void loop() {
    Watchdog.reset();
    delay(5000); //If ISCORail is off , wait 5 seconds every loop
  checkTexts();
  Watchdog.reset();
  checkHydraProbes();
  //checkISCO();
  getBV();
  if (millis() - lastPost > minsToPost * 60000) //Post data every (minsToPost) minutes
  {
    //postData();
    lastPost = millis();
    flushFonaSerial();
  } else
  {
    Serial.print(((minsToPost * 60000) - (millis() - lastPost)) / 1000); Serial.println(" Seconds left b4 Post");
  }

    if (millis() - fonaTimer > resetFona * 60000) //Reset fona every (resetFona) minutes
  {
    toggleFona();
    delay(5000);
    toggleFona();
    fonaTimer = millis(); //reset timer
  } else
  {
    Serial.print(((resetFona * 60000) - (millis() - fonaTimer)) / 1000); Serial.println(" Seconds left b4 Fona reset");
  }
  

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

void checkHydraProbes()
{

  if (moistureSensor.getHPStatus())
  {
    Serial.println("---------------Sensor 1-----------------");
    moistureSensor.parseResponse();


    temp = moistureSensor.getTemp();
    moisture = moistureSensor.getMoisture();
    conductivity = moistureSensor.getConductivity();
    permittivity = moistureSensor.getPermittivity();

    Serial.print("Temp = ");
    Serial.println(temp);
    Serial.print("Moisture = ");
    Serial.println(moisture);
    Serial.print("Conductivity = ");
    Serial.println(conductivity);
    Serial.print("Permittivity = ");
    Serial.println(permittivity);
    Serial.println("");
  }
  else
  {
    Serial.println("Could not communicate with HydraProbe (1).");
    Serial.println("Check Connection.");
  }


  if (moistureSensor2.getHPStatus())
  {
    Serial.println("---------------Sensor 2-----------------");
    moistureSensor2.parseResponse();


    temp2 = moistureSensor2.getTemp();
    moisture2 = moistureSensor2.getMoisture();
    conductivity2 = moistureSensor2.getConductivity();
    permittivity2 = moistureSensor2.getPermittivity();

    Serial.print("Temp 2 = ");
    Serial.println(temp2);
    Serial.print("Moisture 2 = ");
    Serial.println(moisture2);
    Serial.print("Conductivity 2 = ");
    Serial.println(conductivity2);
    Serial.print("Permittivity 2 = ");
    Serial.println(permittivity2);
    Serial.println("");
  }
  else
  {
    Serial.println("Could not communicate with HydraProbe (2).");
    Serial.println("Check Connection.");
  }



  if (moistureSensor3.getHPStatus())
  {

    Serial.println("---------------Sensor 3-----------------");
    moistureSensor3.parseResponse();


    temp3 = moistureSensor3.getTemp();
    moisture3 = moistureSensor3.getMoisture();
    conductivity3 = moistureSensor3.getConductivity();
    permittivity3 = moistureSensor3.getPermittivity();
    Serial.print("Temp 3 = ");
    Serial.println(temp3);
    Serial.print("Moisture 3 = ");
    Serial.println(moisture3);
    Serial.print("Conductivity 3 = ");
    Serial.println(conductivity3);
    Serial.print("Permittivity 3 = ");
    Serial.println(permittivity3);
    Serial.println("");
  }
  else
  {
    Serial.println("Could not communicate with HydraProbe (3).");
    Serial.println("Check Connection.");
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

      if (millis() - deleteTextsTimer > 300000)
      {
        for (int y = 0 ; y < 30 ; ++y)
        {
          deleteSMS(y);
        }
        deleteTextsTimer = millis();
      }

    }
    char Message[200];
    if (commandNum == 1)
    {
      commandNum = -1;
      char message[100];
      int y = sprintf(message, "%s%f%s%f", "Battery Voltage = ", BattVoltage, "\n5V Rail Voltage= ", FiveVoltage);
      sendSMS(senderNum, message);
      Serial.println("Sent voltages to Sender");
    }
   
    if (commandNum == 3)
    {
      Serial.println("Sent measurement to Sender");
      sendHP(senderNum, 1);
      commandNum = -1;
    }
    if (commandNum == 4)
    {
      Serial.println("Sent measurement to Sender");
      sendHP(senderNum, 2);
      commandNum = -1;
    }
    if (commandNum == 5)
    {
      Serial.println("Sent measurement to Sender");
      sendHP(senderNum, 3);
      commandNum = -1;
    }
  
    if (commandNum == 7)
    {
      Serial.println("Toggled HydraProbe Rail");
      if (HPRail)
      {
        toggleHP();
        sendSMS(senderNum, "Turned off HydraProbe");

      } else
      {
        toggleHP();
        sendSMS(senderNum, "Turned on HydraProbe");

      }
      commandNum = -1;
    }
  
    
    if (commandNum == 0)
    {

      sprintf(Message, "%s", "Did not understand the command.\n 1= Rail Voltages \n 3=Hydra1 \n 4=Hydra2 \n 5=Hydra3 \n 7=Turn");
      if (HPRail)

      {
        sprintf(Message, "%s%s", Message, " off ");
      } else
      {
        sprintf(Message, "%s%s", Message, " on ");
      }
      sprintf(Message, "%s%s", Message, "Hydra Probe");
      /*\n 8= Turn");
        if (ISCORail)
        {
        sprintf(Message, "%s%s", Message, " off ");
        } else
        {
        sprintf(Message, "%s%s", Message, " on ");
        }
        sprintf(Message, "%s%s", Message, "ISCO \n 9= Change to");
        if (grabSampleMode)
        {
        sprintf(Message, "%s%s", Message, " AUTO ");
        } else
        {
        sprintf(Message, "%s%s", Message, " GRAB ");
        }
        sprintf(Message, "%s%s", Message, "Sample Mode");
      */

      sendSMS(senderNum, Message);
    }
    else
    {

    }
  } else
  {
    Serial.println("No New SMS");
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
   boolean respond = false;
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


  if (strcmp(senderNum, timsPhone) == 0)
  {
    respond = true;
  }
  if (strcmp(senderNum, annesPhone1) == 0)
  {
    respond = true;
  }
  if (strcmp(senderNum, annesPhone2) == 0)
  {
    respond = true;
  }
  if (strcmp(senderNum, worthsPhone) == 0)
  {
    respond = true;
  }
  if (strcmp(senderNum, katherinesPhone) == 0)
  {
    respond = true;
  }
  if (strcmp(senderNum, googlePhone) == 0)
  {
    respond = true;
  }

  if (respond)
  {
    Serial.println("Accepted number");
    // Retrieve SMS value.
    uint16_t smslen;
    if (! fona.readSMS(number, replybuffer, 250, &smslen)) { // pass in buffer and max len!
      Serial.println("Failed!");
    }
    Serial.print(F("***** SMS #")); Serial.print(number);
    Serial.print(" ("); Serial.print(smslen); Serial.println(F(") bytes *****"));
    Serial.println(replybuffer);
    commandNum = atoi(replybuffer);
    Serial.println(F("*****"));
    return 1;
  } else
  {
    Serial.println("NOT Accepted number");
  }
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

void checkISCO()
{
  if (Serial.available())
  {
    char a = Serial.read();
    
    if (a == 'T')
    {
      toggleFona();
    }
   


  }

}




void sendHP(char*from, int sensor)
{
  Serial.println("In send HP");
  char message[100];
  switch (sensor)
  {
    case 1:
      {
        int y = sprintf(message, "%s%.02f%s%s%.02f", "Temp HP 1 = ", temp , "\n", "Moisture HP 1 =", moisture);
      }
      break;
    case 2:
      {
        int y = sprintf(message, "%s%.02f%s%s%.02f", "Temp HP 2 = ", temp2 , "\n", "Moisture HP 2 =", moisture2);
      }
      break;
    case 3:
      {
        int y = sprintf(message, "%s%.02f%s%s%.02f", "Temp HP 3 = ", temp3 , "\n", "Moisture HP 3 =", moisture3);
      }
      break;
    default:
      break;

  }

  Serial.print("Sending to "); Serial.println(from);
  sendSMS(from, message);

}

void getBV()
{

  float bvVoltSplit = analogRead(BattRail_VIN) * (3.3 / 1023);
  float hpVoltSplit = analogRead(FiveRail_VIN) * (3.3 / 1023);
  //float ThreeVoltSplit = analogRead(ThreeRail_VIN) * (3.3 / 1023);
  Serial.print("Raw Batt voltage = "); Serial.println(bvVoltSplit);
  Serial.print("Raw Hydra voltage = "); Serial.println(hpVoltSplit);
  //Serial.print("Raw 33 voltage = "); Serial.println(ThreeVoltSplit);
  //ThreeVoltage = ThreeVoltSplit;
  BattVoltage = bvVoltSplit * (1000 + 220) / 220; //Resistor Values R1 = 220 , R2 = 1000
  FiveVoltage = hpVoltSplit * (220 + 220) / 220; //Resistor Values R1 = 220 , R2 = 1000
  Serial.print("Batt voltage = "); Serial.println(BattVoltage);
  Serial.print("5V voltage = "); Serial.println(FiveVoltage);
  //Serial.print("33 voltage = "); Serial.println(ThreeVoltage);
}

void toggleHP()
{
  if (HPRail)
  {
    digitalWrite(HPRelay, LOW);
    HPRail = false;
  } else
  {
    HPRail = true;
    digitalWrite(HPRelay, HIGH);
  }
}
void toggleRQ30()
{
  if (RQ30)
  {
    RQ30 = false;
    digitalWrite(RQ30Relay, LOW);
  } else
  {
    RQ30 = true;
    digitalWrite(RQ30Relay, HIGH);
  }
}


/*void postData()
{
  if (! client.init(*GPRSSerial)) {
    Serial.println(F("Couldn't find FONA"));
    while (1);
  } else
  {

    client.addFloat("moisture1", moisture * 100, "%");
    client.addFloat("moisture2", moisture2 * 100, "%");
    client.addFloat("moisture3", moisture3 * 100, "%");
    client.addFloat("temp1", temp, "degC");
    client.addFloat("temp2", temp2, "degC");
    client.addFloat("temp3", temp3, "degC");
    //client.addInt("Bottle Number", getBottleNumber(), "/24");
    client.addFloat("Battery Voltage", BattVoltage, "V");
    //client.addInt("Level Reading", levelReading, "/1024");
    //client.add(VARIABLE_LABEL_2, humidity);
    ///client.add(VARIABLE_LABEL_3, pressure);
    client.sendAll(unit, GPS);
    client.clearData();
    Serial.println("Reached end of postData");
    Serial.println("cleared ISCO Serial");
  }
}

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
*/

void saveData() {

  if (writeNum == 0)
  {
    //buildDateTime();
   // setfileName();
  }

  dtostrf(conductivity, 3, 2, Conductivity1Char);
  dtostrf(conductivity2, 3, 2, Conductivity2Char);
  dtostrf(conductivity3, 3, 2, Conductivity3Char);
  dtostrf(permittivity, 3, 2, Permittivity1Char);
  dtostrf(permittivity2, 3, 2, Permittivity2Char);
  dtostrf(permittivity3, 3, 2, Permittivity3Char);
  dtostrf(moisture, 3, 2, Moisture1Char);
  dtostrf(moisture2, 3, 2, Moisture2Char);
  dtostrf(moisture3, 3, 2, Moisture3Char);
  dtostrf(temp, 3, 2, Temp1Char);
  dtostrf(temp2, 3, 2, Temp2Char);
  dtostrf(temp3, 3, 2, Temp3Char);
  dtostrf(BattVoltage, 3, 2, bVChar);


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

      myFile.println("TIMESTAMP(EST),Ambient Temperature(deg C),temp1 (degc),temp2 (degc),temp3 (degc),moisture1 (%),moisture2 (%),moisture3 (%),permittivity,permittivity2,permittivity3,Battery Voltage (V)");
    }
    delay(10);
    buildDateTime();
    myFile.print(dateTime);
    myFile.print(",");
    myFile.print(Temp1Char);
    myFile.print(",");
    myFile.print(Temp2Char);
    myFile.print(",");
    myFile.print(Temp3Char);
    myFile.print(",");
    myFile.print(Moisture1Char);
    myFile.print(",");
    myFile.print(Moisture2Char);
    myFile.print(",");
    myFile.print(Moisture3Char);
    myFile.print(",");
    myFile.print(Permittivity1Char);
    myFile.print(",");
    myFile.print(Permittivity2Char);
    myFile.print(",");
    myFile.print(Permittivity3Char);
    myFile.print(",");
    myFile.print(bVChar);
    myFile.println(" ");
    // close the file:
    myFile.close();
    Serial.println("done.");
  }


}

/*
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

/*  code to process time sync messages from the serial port   
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
*/
