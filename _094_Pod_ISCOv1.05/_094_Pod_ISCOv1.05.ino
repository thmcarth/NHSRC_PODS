#include <Adafruit_INA260.h>

#include <HTTPS_VIPER.h>
//#include <Adafruit_FONA.h>
#include <Timezone.h>
#include <Time.h>
#include <TimeLib.h>
//#include <Ubidots_FONA.h>


/* * Teensy 3.5 Custom PCB developed to automate ISCO 6700 Sampling with a digital output pin
 * Unit is controlled via SMS commands
 * Bottle number and time sampled is parsed from ISCO Serial Output
 * Data including bottle number, water level and fixed GPS coordinates are streamed to VIPER database using HTTP Post from a FONA 808 cellular module
 * Time, bottle number and water level are saved to SD card locally
 * Water level (presence) is detected by an optical sensor (Optomax Digital Liquid Level Sensor)
 * Auto Sampling Mode - Pulses pin every (grabSampleInterval) minutes when water is present in the collection basin
 * Grab Sampling Mode - Pulses pin whenever a grab sample is initiated by SMS command


  Tim McArthur

*/
//094 Pod code Version 0.1 - Cleaned up working code and added watchdog to testing unit. 2/21/18 - Tim McArthur
//Version 1.0  - Cleaned up variable/function names and logic. 4/11/2018 - Tim McArthur
//Version 1.01 - Added static set of numbers to accept commands from
//             - Default starts in grab mode
//             - Grab sample ignores water level. Still depends on ISCO being enabled
//Version 1.02 - Change auto sampling interval via text message
//Version 1.03 - Saving sampling mode and interval to EEPROM to hold on restart - 4/25/2018 - Tim McArthur
//             - Writing more information to SD card\
//Version 1.04 - Daylight Savings time, confirm Google number functions properly.



//GPS and unit values iused in VIPER Post

char GPS[] = "40.5087450, -74.3580650 0"; //needs a (space 0) at the end, example ="35.068471,-89.944730 0"
int unit = 6;
char versionNum[] = "_094_Pod_ISCOv1.05";

//////////////////////////////////


// DAYLIGHT SAVINGS TIME
TimeChangeRule myDST = {"EDT", Second, Sun, Mar, 2, -240};    // Daylight time = UTC - 4 hours
TimeChangeRule mySTD = {"EST", First, Sun, Nov, 2, -300};     // Standard time = UTC - 5 hours
Timezone myTZ(myDST, mySTD);
/////////

//Watchdog
#include <Adafruit_SleepyDog.h>
//EEPROM
#include <EEPROM.h>
int eepromModeAddr = 0;
int eepromIntervalAddr = 1;

//////////////////////////////////////////////

//Char arrays to hold time information

char monthChar[5];
char dateChar[5];
char hourChar[5];
char minuteChar[5];
char secondChar[5];
boolean isDST = false;
///////////////////////////////////////

//Char arrays to hold measurement values for posting to VIPER
char Temp1Char[10];
char Temp2Char[10];
char Temp3Char[10];
char levelChar[10];
char bVChar[10];

///////////////////////////////////////
//Static numbers to text when bottles are full and accept SMS comands from:
char* hawesPhone = "+12524126262";
//Numbers to accept SMS commands from:
char* annesPhone1 = "+19198860812";
char* annesPhone2 = "+15179451531";
char* katherinesPhone = "+16157148918";
char* worthsPhone = "+19194233837";
char* googlePhone = "+19192301472";
char* garrettPhone = "+19196019412";
bool successTimText = false; //logic to control when to text bottle full message
int readSMSSuccess = 0;
int deleteSMSSuccess = 0;
int sender_count = 0;
/////////////////////////

/////////////////////////INA 260 CODE 

/////////////////////////

//DAVIS RAINBUCKET
#define DAVIS 0
///////////////I2C Comms
#include <Wire.h>
int Intensity_period = 30; // Intensity period and how often we want to ask device for rainfall intensity.
unsigned long UpdateRate = 4000; //Number of ms between serial prints, 4 seconds by default
uint8_t ADR = 0x08; //Address of slave device, 0x08 by default
long rain_period = Intensity_period;
long previousMillis = 0; 

//INA260 comms
int INA = B10000000; //connect
uint8_t INA_I = 0x01;
uint8_t INA_V = 0x02;
//////////////END I2C Inits

//FONA HTTP
HTTPS_VIPER http = HTTPS_VIPER(); 
char senderNum[30];    //holds number of last number to text SIM

char replybuffer[255]; //holds Fonas text reply
char * replybuffer_command; //holds Fonas command part
char * replybuffer_interval; //holds Fonas interval part
HardwareSerial *xbeeSerial = &Serial5;
int commandNum = -1; //integer value for which text command has been sent
unsigned long lastPost = millis();
int minsToPost = 5;
//Timing for Samples
boolean sample_occurred = false; // goes true when a sample occurs
long sample_start = 0;  // keeps time for sample start
long sample_stop = 0; // once this 
long sample_stop_time = 5;  // 5 minutes
long sample_period = 8;// 8 hours
int  samples_allowed = 2;
////////////////////////////

//Voltage Readings
#define BattRail_VIN A1 //Pin 15
float BattVoltage;
#define FiveRail_VIN A9
float FiveVoltage;
//#define ThreeRail_VIN A5
#define levelPin A20
//float ThreeVoltage;
///////////////////////

//SD
char bottleNumChar[10];
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

//ISCO
#include <Time.h>
#define IscoSamplePin 16
char iscoData[500];
int cdIndex = 0;
bool grabSampleMode;
int grabSampleInterval; //Minute interval to auto grab sample as long as water level is high enough
char iscoTime[15]; //holds last bottle sample time from ISCO memory
#define iscoSerial Serial4
bool timerOn = false;//logic for texting after sample is complete
unsigned long textTimer = millis();
unsigned long sampleTimer = millis();
bool ISCORail = true;
int levelReading;
unsigned long iscoTimeout = millis();
/////////////////ISCO


///////////////////////////RELAYS
bool RQ30 = true;
//Soil moisture is on pin 30
#define HPRelay 27 //Outpin PIN to control HydraProbe 12V Rail
#define RQ30Relay 26 //Output PIN to control RQ30 12V Rail
unsigned long fonaTimer = millis(); //timer to reset Fona
//////////////////////////RELAYS


void setup() {
  Adafruit_INA260 ina260 = Adafruit_INA260();
  Serial.begin(9600);
  int countdownMS = Watchdog.enable(300000); //300 seconds or 5 minutes watchdog timer
  Serial.print("Enabled the watchdog with max countdown of ");
  Serial.print(countdownMS, DEC);
  Serial.println(" milliseconds!");
  Serial.println();

// SETUP DST
    setTime(myTZ.toUTC(compileTime()));
//
 
  delay(2000);
  setSyncProvider(getTeensy3Time);
  Serial.print("Initializing SD card...");  //startup SD card

  if (!SD.begin(chipSelect)) {
    Serial.println("initialization failed!");
  }
  Serial.println("SD Card initialization done.");
  Serial.print("Code version ");Serial.println(versionNum);

  pinMode(HPRelay, OUTPUT); //Set up HydraProbe relay
 
  //pinMode(levelPin, INPUT);
  delay(50);
  
  pinMode(BattRail_VIN, INPUT);
  digitalWrite(HPRelay, HIGH); //turn on HydraProbe 12V Rail
  Watchdog.reset();
  EEPROM.get(eepromModeAddr, grabSampleMode); //Get Sampling mode from EEPROM
  EEPROM.get(eepromIntervalAddr, grabSampleInterval); //Get sampleInterval from EEPROM
  Serial.print("Interval is ");Serial.print(grabSampleInterval);Serial.println("minutes");
  //xbeeSerial->begin(19200); //Startup SMS and Cell client
  
  pinMode(IscoSamplePin, OUTPUT);  // Setup sample pin output
  digitalWrite(IscoSamplePin, LOW);
  Serial.begin(9600);
  iscoSerial.begin(9600); //Setup comms with ISCO
  Watchdog.reset();
}

void loop() {
    Watchdog.reset();
  if (ISCORail) // this is true at start
  {
    //Serial.print("Started waiting for ISCO...Millis = "); Serial.println(millis()); //If ISCO is enabled, reset the watchdog while waiting for data on ISCO Serial
    do {
      Watchdog.reset();
    } while (iscoSerial.available() == 0 && millis() - iscoTimeout < 40000); //If there is data on the serial line, OR its been 40 seconds, continue
    iscoTimeout = millis(); //reset timer
    Serial.print("Finished waiting for ISCO...Millis = "); Serial.println(millis());
    do {  //read in data from ISCO Serial
      readIscoSerial();
    } while (iscoSerial.available() > 0);
  } else
  {
    delay(5000); //If ISCORail is disabled , wait 5 seconds every loop
  }
  checkTexts(); //check for SMS commands
  Watchdog.reset(); //ensure the system doesn't prematurely reset
  checkHydraProbes();
  checkUserInput(); //check Serial for input commands
  getBV(); //get voltage rail readings
  if (millis() - lastPost > minsToPost * 60000) //Post data every (minsToPost) minutes
  { // at the moment we post every 1 minute
    postData();
    lastPost = millis(); //reset timer
    clearIscoSerial(); //clear isco and fona serials
    flushxbeeSerial();
  } else
  {
    Serial.print(((minsToPost * 60000) - (millis() - lastPost)) / 1000); Serial.println(" Seconds left b4 Post");
  }



#if DAVIS
//>>>>>>> 6b4848fa0c3169bf08481d0b8e37c27756e02b87
unsigned long currentWaterMillis = millis();
if(currentWaterMillis - previousMillis > rain_period) {
    // save the last time you blinked the LED 
    previousMillis = currentWaterMillis;  
    I2C_rain();
  } 
#endif 

  if (millis() - lastSave > minsToSave * 60000) //Save data every (minsToSave) minutes (1 mins)
  {
    Serial.println("----------");
    Serial.println("SAVING");
    Serial.println("----------");
    saveData();
    lastSave = millis();
  }
  if (grabSampleMode) //Display sample mode on Serial Monitor 
  {
    Serial.println("------------------Currently in GRAB Sample Mode--------------------");
  } else
  {
    Serial.println("------------------Currently in AUTO Sample Mode--------------------");
  }

  if (getWaterLevel()) //If there is water in the pan......
  {
    Serial.println(" Water detected in Pan");
    if (!grabSampleMode) // ....AND.... we are in auto sample mode.....
    {
      if (millis() - sampleTimer > 60000 * grabSampleInterval) //.....AND....it's time to sample. Then sample.
      {
        Serial.println("Timer Finished!");
        sampleTimer = millis();
        if (ISCORail) //if true collect a sample
        {
          toggleSample(); // collect if available 
        }
      } else
      {
        Serial.print("Sample timer has "); Serial.print(((60000 * grabSampleInterval) - (millis() - sampleTimer)) / 1000); Serial.println("Seconds remaining");
      }
    } else
    {
      //Serial.println("------------------Currently in grab Sample Mode--------------------");
    }
  } else
  {
    Serial.println(" No Water detected in Pan");
  }

  iscoData[cdIndex] = '\0'; //null out data
  cdIndex = 0;



  if (millis() - textTimer > 100000 && timerOn) //Send last number to text a status on sampling after 100 second delay from sample command
  {
    //sendSampleReply(senderNum);
    timerOn = false;
  }

  for (int x = 0; x < 64 ; ++x)
  { 
    char a = Serial.read();
    char b = xbeeSerial->read();
  }

  clearIscoSerial();
  cleararray(iscoData);
}

void checkHydraProbes(){


}

void checkTexts() //reads SMS(s) off the Fona buffer to check for commands
{
  int ind = 0;
  String msg;
 while (xbeeSerial->available()){
 msg[ind++]=xbeeSerial->read();
 }
 
 if (msg == "C1"){
 
 char* batt = getBV(); // call for battery
 sendSMS(batt);
 }

 if (msg =="C2"){
  
 }
 
 if (msg =="C3"){
  
 }
 
if (msg =="C4"){
  
 }
 
 if (msg =="C5"){

 }
}




///////////////////////////////***********************Xbee FUNCTIONS
/*** SMS ***/
char* readSMSNum() {  //read in the number of text messages on the buffer and send back
  // read the number of SMS's!
  int smsnum;
  char response[50];
  flushxbeeSerial();
  //Serial.println("Writing AT Command to Serial 1");
  delay(50);
  int index = 0;
  char num;
  while (xbeeSerial->available())
  {
    num = xbeeSerial->read();
    response[index] = num;
  }
  
    return response;
  }




int readSMS(int8_t number) { //read the SMS into the replybuffer and parse the command number
  boolean respond = false;
  // read an SMS
  //Serial.print(F("Read #"));
  Serial.print(F("\n\rReading SMS #")); Serial.println(number);  // Retrieve SMS sender address/phone number.

}

int deleteSMS(int8_t number) { //delete the SMS number
 return 1;

}

bool sendSMS(char* message) {
  // send an SMS!
 xbeeSerial->print("C");
  xbeeSerial->println(message);
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

bool sendXbee(char* message)
{
  char a;
  flushxbeeSerial();
  xbeeSerial->println(message);
  while (xbeeSerial->available())
    a = xbeeSerial->read();
  if (a == '0' || a == false)
  {
    return false;
  }
  if (a == '1' || a == true)
  {
    return true;
  }

}

void flushxbeeSerial() { // flush xbee Serial port
  while (xbeeSerial->available())
    xbeeSerial->flush();
}

///////////////////////////////////////**********************FONA FUNCTIONS

void checkUserInput()
{
  if (Serial.available())
  {
    char a = Serial.read();
    if (a == 'S')
    {
      toggleSample();
    }
    if (a == 'T')
    {
    }
    if (a == 'Q')
    {
      sendQuery();
    }
    if (a == 'G')
    {
      delay(200);
      if (getBottleNumber() == 0)
      {
        Serial.println("No communication with ISCO. Could not find data");
      } else
      {
        Serial.print("Sampled -- Bottle ");
        Serial.print(getBottleNumber());
        if (getBottleNumber() == 24)
        {
          sendSMS("All bottles are full. ISCO Sampler must be reset and bottles replaced");
        }
        unsigned long sampled = getSampledTime();
        //Serial.print("time_t=");Serial.println(sampled);
        Serial.print(" @ ");
        Serial.print(month(sampled)); Serial.print("/"); Serial.print(day(sampled)); Serial.print("/"); Serial.print(year(sampled)); Serial.print("  "); Serial.print(hour(sampled)); Serial.print(":"); Serial.print(minute(sampled)); Serial.print(":"); Serial.println(second(sampled));
      }
    }


  }

}



void toggleSample()
{
  int botNum;
  char Message[50];
  if (getBottleNumber() == 24)
  {
    if (!successTimText)
    {
      successTimText = sendSMS("All bottles are full. ISCO Sampler must be reset and bottles replaced. Will not sample");
    }
   }
  {
    Serial.println("Start sampler");
    textTimer = millis();
    timerOn = true;

    {
      botNum = getBottleNumber() + 1;
      if (botNum > 24)
      {
        botNum = 1;
      }
      if (grabSampleMode)
      {
        if (botNum == 2)
        {
          successTimText = false;
        }
        sprintf(Message, "%s%i", "Received Sample Command. Now Sampling Bottle # ", botNum);
        sendSMS(Message);
      }
      Sample();
    }
  }
}

void Sample()
{
  
  Serial.println("Sending signal to ISCO");
  digitalWrite(IscoSamplePin, HIGH);
  delay(3000);
  digitalWrite(IscoSamplePin, LOW);

}

void sendQuery() //Function to establish comms with ISCO if needed
{
  Serial.println("Sending query to ISCO");
  iscoSerial.write('?');
  iscoSerial.write(13);
  delay(2000);
  if (!iscoSerial.available())
  {
    iscoSerial.write('?');
    iscoSerial.write(13);
    delay(2000);
    Serial.println("Trying another baud...");
  } else
  {
    readIscoSerial();
  }
  if (!iscoSerial.available())
  {
    iscoSerial.write('?');
    iscoSerial.write(13);
    delay(10);
    Serial.println("3rd baud attempt...");
  } else
  {
    readIscoSerial();
  }
  delay(2000);
  if (!iscoSerial.available())
  {
    Serial.println("No data available...giving up");
  }

}

void cleararray(char* a){
  for (int x =0; x < sizeof(a) / sizeof(a[0]); x++)
  {
  a[x] = 0;
  }
}

void clearIscoSerial()
{
  for (int x = 0; x < 64 ; ++x)
  {
    char a = iscoSerial.read();
  }
}


void readIscoSerial() //CHECK FOR OVERFLOW ON THIS ARRAY
{
  unsigned long timer = millis();
  do
  {
    if (iscoSerial.available())
    {
      char a = iscoSerial.read();
      if (cdIndex < sizeof(iscoData))
      {
        iscoData[cdIndex] = a; //here might need to ring buffer
      }
      ++cdIndex;
    }


  } while (millis() - timer < 1000);
}
int getBottleNumber() //parse the bottle Number from ISCO input
{
  int commaNumber1 = 0;
  int bottleNumber = 0;
  sprintf(bottleNumChar,"%s","");
  for (int x = 0; x < 210 ; ++x)
  {
    if (iscoData[x] == ',')
    {
      ++commaNumber1;
      //Serial.print(commaNumber);
    }
    if (commaNumber1 == 10)
    {
      int y = sprintf(bottleNumChar, "%c%c", iscoData[x + 2], iscoData[x + 3]);
      bottleNumber = atoi(bottleNumChar);
      Serial.print("Next four characters are"); Serial.print(iscoData[x + 1]); Serial.print(iscoData[x + 2]); Serial.print(iscoData[x + 3]); Serial.println(iscoData[x + 4]);
      Serial.print("Bottle Number is "); //Your next line will be...
      //Serial.print(bottleNumber);
      //return bottleNumber;
      return bottleNumber;
    }

  }
  return 0;
}


unsigned long getSampledTime() //get time time the last bottle was sampled
{
  int commaNumber2 = 0;
  float sampledTime;
  for (int x = 0; x < 210 ; ++x)
  {
    if (iscoData[x] == ',')
    {
      ++commaNumber2;
      //Serial.print(commaNumber);
    }
    if (commaNumber2 == 11)
    {
      int startTimeIndex = x + 1;

      for (int y = 0; y < 11; ++y)
      {
        iscoTime[y] = iscoData[startTimeIndex];
        ++startTimeIndex;
      }
      Serial.println(iscoTime);
      double sampledTime = atof(iscoTime);
      time_t unixTime;
      Serial.print("Sampled time "); Serial.println(sampledTime, 6);
      unixTime = (sampledTime - 25569) * 86400;
      Serial.print("unix time "); Serial.println(unixTime);
      return unixTime;

      //Serial.print("Next two characters are");Serial.print(iscoData[x+1]);Serial.println(iscoData[x+2]);
      //Serial.print("Bottle Number is ");Serial.println(bottleNumber);

    }

  }
  return 0;
}


char* getBV()
{
  /*
   * int INA = B100;00000; //connect
     uint8_t INA_I = 0x01;
     uint8_t INA_V = 0x02;
   */
 char* c = "0";
  return c;
  
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


bool getWaterLevel()
{
  int reading = 0;
  int over = 0;
  int under = 0;
  for (int x = 0 ; x < 50 ; ++x)
  {
    reading = analogRead(levelPin);
    if (reading > 100)
    {
      ++over;
    } else
    {
      ++under;
    }
  }
  if (over + 20 > under) //bias towards failing dry
  {
    return false;
  } else
  {
    return true;
  }

}

void postData() //post Data to VIPER
{
    int intWL;
    if (getWaterLevel()) //Transpose the water level to 50 (wet) or 0 (dry) for viewing on VIPER
    {
      intWL = 50;
      levelReading = 50;
    } else
    {
      intWL = 0;
      levelReading = 0;
    }
    http.addInt("Bottle Number", getBottleNumber(), "/24");
    http.addFloat("Battery Voltage", BattVoltage, "V");
    http.addInt("Level Reading", intWL, " 0/50");

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

  dtostrf(BattVoltage, 3, 2, bVChar);

  sprintf(levelChar, "%i", levelReading);

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

      myFile.println("TIMESTAMP(EST),BottleNumber (/24),level Reading (/1024),Battery Voltage (V)");
    }
    delay(10);
    buildDateTime();
    myFile.print(dateTime);
    myFile.print(",");
    myFile.print(bottleNumChar);
    myFile.print(",");
    myFile.print(levelChar);
    myFile.print(",");
    myFile.print(bVChar);
    myFile.println(" ");
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



float getWaterIntensity() //parse the bottle Number from ISCO input
{
  char waterString [10];
  int commaNumber1 = 0;
  float water_intensity = 0;
  sprintf(waterString,"%s","");
  for (int x = 0; x < 210 ; ++x)
  {
    if (iscoData[x] == ',')
    {
      ++commaNumber1;
      //Serial.print(commaNumber);
    }
    if (commaNumber1 == 12)//We do not know this yet
    {
      float y = sprintf(waterString, "%c%c%c%c", iscoData[x + 2], iscoData[x + 3], iscoData[x + 4], iscoData[x+5]);
      water_intensity = atoi(bottleNumChar);
      Serial.print("Your next line will be "); Serial.print(iscoData[x + 1]); 
      Serial.print(iscoData[x + 2]); Serial.print(iscoData[x + 3]); Serial.println(iscoData[x + 4]);
      Serial.print("Water intensity is ");
      Serial.println(water_intensity);
      //return bottleNumber;
      return water_intensity;
    }

  }
  return 0;
}


/*
 * THE FUNCTIONS BEYOND THIS POINT ARE UPDATES TO THE ISCO'S FUNCTIONALITY 
 */

/*
 * Daylight ST checks the clock of the Teensy and moves the hour to Daylight Savings if valid here
 * No params
 * 
 */
void DaylightST(){ //check to see if DST is active
  time_t eastern, utc;
TimeChangeRule *tcr;
TimeChangeRule usEDT = {"EDT", Second, Sun, Mar, 2, -240};  //UTC - 4 hours
TimeChangeRule usEST = {"EST", First, Sun, Nov, 2, -300};   //UTC - 5 hours
Timezone usEastern(usEDT, usEST);
utc = now();  //current time from the Time Library
eastern = usEastern.toLocal(utc, &tcr);
Serial.print("The time zone is: ");
Serial.println(tcr -> abbrev);
}
/*
 * compileTime helps the clock account for the time lag during compile/startup time
 * no params
 */
time_t compileTime()
{
    const time_t FUDGE(10);     // fudge factor to allow for compile time (seconds, YMMV)
    const char *compDate = __DATE__, *compTime = __TIME__, *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
    char chMon[3], *m;
    tmElements_t tm;

    strncpy(chMon, compDate, 3);
    chMon[3] = '\0';
    m = strstr(months, chMon);
    tm.Month = ((m - months) / 3 + 1);
    tm.Day = atoi(compDate + 4);
    tm.Year = atoi(compDate + 7) - 1970;
    tm.Hour = atoi(compTime);
    tm.Minute = atoi(compTime + 3);
    tm.Second = atoi(compTime + 6);
    time_t t = makeTime(tm);
    return t + FUDGE;           // add fudge factor to allow for compile time
}

void INA_read(){
/*
 * 
//INA260 comms
int INA = B10000000; //connect
uint8_t INA_I = 0x01;
uint8_t INA_V = 0x02;

All data bytes are transmitted most significant byte first.
 */
   uint8_t Byte1 = 0; //Bytes to read then concatonate
  uint8_t Byte2 = 0;
   int * address = INA;
   double divisor = 800.0;
  Wire.beginTransmission(INA);
  Wire.write(INA);
  delay(1);
  Wire.write(INA_I);

}
#if DAVIS 
void I2C_rain(){

   unsigned int tips = 0; //Used to measure the number of tips
  uint8_t Byte1 = 0; //Bytes to read then concatonate
  uint8_t Byte2 = 0;

  Wire.requestFrom(ADR, 2);    // request 2 bytes from slave device #8
  
  Byte1 = Wire.read();  //Read number of tips back
  Byte2 = Wire.read();

  tips = ((Byte2 << 8) | Byte1); //Concatenate bytes

  Serial.print("Intensity last read = ");

  if (tips == 65535)
  {
      Serial.println("Sensor not connected");  // tips == 65535 if I2C isn't connectected
  }
  else
  {
      double inch = tips/100;
      tips = tips/Intensity_period;
      Serial.println(tips);  //Prints out tips to monitor
  }
}
#endif
