 #include <Adafruit_INA260.h>
#include <HTTPS_VIPER.h>
//#include <Adafruit_FONA.h>
#include <HydraProbe.h>
#include <Timezone.h>
#include <Time.h>
#include <TimeLib.h>
//#include <Ubidots_FONA.h>
#include <SoftwareSerial.h>

/* * Teensy 3.5 Custom PCB developed to automate ISCO 6700 Sampling with a digital output pin
 * Unit is controlled via SMS commands
 * Bottle number and time sampled is parsed from ISCO Serial Output
 * Data including bottle number, water level and fixed GPS coordinates are streamed to VIPER database using HTTP Post from a FONA 808 cellular module
 * Time, bottle number and water level are saved to SD card locally
 * Water level (presence) is detected by an optical sensor (Optomax Digital Liquid Level Sensor)
 * Auto Sampling Mode - Pulses pin every (grabSampleInterval) minutes when water is present in the collection basin
 * Grab Sampling Mode - Pulses pin whenever a grab sample is initiated by SMS command


Hawes Collier
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
int unit = 1;
char versionNum[] = "_094_Pod_ISCOv1.07";

//////////////////////////////////


// DAYLIGHT SAVINGS TIME
TimeChangeRule myDST = {"EDT", Second, Sun, Mar, 2, -240};    // Daylight time = UTC - 4 hours
TimeChangeRule mySTD = {"EST", First, Sun, Nov, 2, -300};     // Standard time = UTC - 5 hours
Timezone myTZ(myDST, mySTD);
/////////

//Watchdog
//#include <Adafruit_SleepyDog.h>
//EEPROM
#include <EEPROM.h>
int eepromModeAddr = 0;
int eepromIntervalAddr = 1;
int eepromfirst_upload = 2;
int is_first = 0;
#define FIRST 1
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

char* googlePhone = "+19192301472";
bool successTimText = false; //logic to control when to text bottle full message
int readSMSSuccess = 0;
int deleteSMSSuccess = 0;
int sender_count = 0;
/////////////////////////

/////////////////////////Parsivel 
int lastParsivel;
int Parsiveltime;
const int SSERIAL_RX_PIN = 10;  //Soft Serial Receive pin
const int SSERIAL_TX_PIN = 11;  //Soft Serial Transmit pin
const int SSERIAL_CTRL_PIN= 3;   //RS485 Direction control
const int RS485_TRANSMIT = HIGH;
const int RS485_RECEIVE = LOW;
String parsivel_data;
char* parsivel_intensity;
SoftwareSerial ParsivelSerial(SSERIAL_RX_PIN, SSERIAL_TX_PIN); // RX, TX
int byteReceived;
int byteSent;
/////////////////////////

//DAVIS RAINBUCKET
float inch_tips;
#define DAVIS 0
///////////////I2C Comms
#include <Wire.h>
int Intensity_period = 30; // Intensity period and how often we want to ask device for rainfall intensity.
unsigned long UpdateRate = 4000; //Number of ms between serial prints, 4 seconds by default
uint8_t ADR = 0x08; //Address of slave device, 0x08 by default
long rain_period = 30000;
long previousMillis = 0; 
int rain_level = 1; //1 is no rain, 2 is little, 3 is lots of rain 
int droplet_pin = A14; // pin 33
//INA260 comms Placeholder
//////////////END I2C Inits


//Hydraprobes
#define HydraSerial Serial3
unsigned long lastProbe = millis();
int Probetime = 60000;
float temp= 0, moisture= 0, conductivity = 0,permittivity = 0;
float temp2= 0, moisture2= 0, conductivity2= 0,permittivity2 = 0;
float temp3= 0, moisture3= 0, conductivity3= 0,permittivity3 = 0;
HydraProbe moistureSensor;  //define data Pin in Header (library .h file)
HydraProbe moistureSensor2;
HydraProbe moistureSensor3;
//
//FONA HTTP
HTTPS_VIPER http = HTTPS_VIPER(); 
char senderNum[30];    //holds number of last number to text SIM
char replybuffer[255]; //holds Fonas text reply
char * replybuffer_command; //holds Fonas command part
char * replybuffer_interval; //holds Fonas interval part
#define xbeeSerial Serial1
#define parsivelSerial Serial2
int commandNum = -1; //integer value for which text command has been sent
unsigned long lastPost = millis();
float minsToPost = 5;
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

//EEPROM timing
unsigned long ident_save = millis();
int timer_ident = 60; //mins
bool first_eeprom = true;
int ident_ADR = 4;
int ident = 1;
//

//ISCO
#include <Time.h>
#define IscoSamplePin 16
int bottle_limit = 4;
char iscoData[500];
int cdIndex = 0;
bool grabSampleMode = true;
int grabSampleInterval = 2 ; //Minute interval to auto grab sample as long as water level is high enough
char iscoTime[15]; //holds last bottle sample time from ISCO memory
#define iscoSerial Serial4
bool timerOn = false;//logic for texting after sample is complete
unsigned long textTimer = millis();
unsigned long sampleTimer = millis();
bool ISCORail = true;
int levelReading;
unsigned long iscoTimeout = millis();
/////////////////ISCO

bool SDcard = true;
///////////////////////////RELAYS
bool RQ30 = false;
//Soil moisture is on pin 30
#define HPdata 30
#define HPRelay 27 //Outpin PIN to control HydraProbe 12V Rail
#define RQ30Relay 26 //Output PIN to control RQ30 12V Rail
//unsigned long fonaTimer = millis(); //timer to reset Fona
unsigned long lastRQ = millis();
unsigned long RQTime = 60000*5;
//////////////////////////RELAYS
Adafruit_INA260 ina260 = Adafruit_INA260();

// SMS flags
bool rec = false;


void setup() {
  Serial.begin(9600);
  //int countdownMS = Watchdog.enable(600000); //600 seconds or 10 minutes watchdog timer
  //Serial.print("Enabled the watchdog with max countdown of ");
  //Serial.print(countdownMS, DEC);
  //Serial.println(" milliseconds!");
  //Serial.println();
  // SETUP DST
    setTime(myTZ.toUTC(compileTime()));  

  delay(2000);
  setSyncProvider(getTeensy3Time);
  Serial.print("Initializing SD card...");  //startup SD card

  if (!SD.begin(chipSelect)) {
    SDcard = false;
    Serial.println("initialization failed!");
  }
  Serial.println("SD Card initialization done.");
  Serial.print("Code version ");Serial.println(versionNum);

  pinMode(HPRelay, OUTPUT); //Set up HydraProbe relay
 
  //pinMode(levelPin, INPUT);
  delay(50);
  
  pinMode(BattRail_VIN, INPUT);
  digitalWrite(HPRelay, HIGH); //turn on HydraProbe 12V Rail
 // //Watchdog.reset();
  //setup_parsivel();// Setup Parsivel output string
  
 /* #if FIRST  //change FIRST to 1 when uploading and running first time
  EEPROM.write(eepromfirst_upload, 0);
  is_first = 1;
  #endif
 if (!is_first)
 EEPROM.get(eepromfirst_upload, is_first);
 
  if(!is_first)
  { // if not fresh upload, take old values
 //EEPROM.get(eepromModeAddr, grabSampleMode); //Get Sampling mode from EEPROM
  EEPROM.get(eepromIntervalAddr, grabSampleInterval); //Get sampleInterval from EEPROM
  }
  else {
    grabSampleMode = false;
    grabSampleInterval = 1;
  }
  */



  
////////////////////////// FIRST TIME UPLOAD, GET THE IDENTITY FOR THIS WETBOARD SET AT 1 OR 0
  int first_up = 0;

  if (first_up){
    EEPROM.write(ident_ADR,ident); // This sets the identity number we will be sending over to the Xbee to prevent the VIPER database from overlapping old data.  This updates every 2 hours onto EEPROM atm
  }
  else{
    ident = EEPROM.read(ident_ADR); // This reads the identity number we will be sending over to the Xbee to prevent the VIPER database from overlapping old data.  This updates every 2 hours onto EEPROM atm
  }




  // if grabSample is in true this doesn't do anything

  
  Serial.print("Interval is ");Serial.print(grabSampleInterval);Serial.println("minutes");

  
  xbeeSerial.begin(9600); 
  //Startup SMS and Cell client DO NOT CHANGE THIS BAUD RATE UNLESS YOU CHANGE XBEE BAUD RATE WITH XCTU
  //  Future design notes for Wiley and team: 
  //  If you can setup serial1 for the alternative UART that Xbee uses with its UART class, we may have more fine control functionality
  //  At the moment, we can only use the Xbee as so: 
  // 1. Change baud rate to speed up slow down serial comms
  // 2. read in data
  // 3. write out data
  // We want to be able to do more functions on data later if this ever gets a 2.0


  
  pinMode(IscoSamplePin, OUTPUT);  // Setup sample pin output to make ISCO sample when function is called to flip this pin
  digitalWrite(IscoSamplePin, LOW);
  Serial.begin(115200);
  iscoSerial.begin(9600); //Setup comms with ISCO
  //parsivelSerial.begin(19200);
  //Watchdog.reset();
  //delay(5000);
  //massSMS("Testing WET Board mass Text (from Teensy)");
  moistureSensor.debugOn(); //Can turn debug on or off to see verbose output (OR NOT)
  moistureSensor.begin(1);  //Red tape
  moistureSensor2.debugOn(); //Can turn debug on or off to see verbose output (OR NOT)
  moistureSensor2.begin(2);  //yellow tape
  moistureSensor3.debugOn(); //Can turn debug on or off to see verbose output (OR NOT)
  moistureSensor3.begin(3);  // green tape
  Serial.println(moistureSensor.getAddress());
  Serial.println(moistureSensor2.getAddress());
  Serial.println(moistureSensor3.getAddress());
/*
 * 01234567
 * 02134567
 * 03142567
 */
   
    
  if (!ina260.begin()) {  // Check to see if  I2c on XBEE is on if there is no comms.  This messed up the INA.
    Serial.println("Couldn't find INA260 chip");  
  }
  else{
  Serial.println("Found INA260 chip!"); 
  }
}
void loop() {
  
  rec = false;
  
  unsigned long currentTime = millis();

  
    //Watchdog.reset();
    checkTexts(); //check for SMS commands

    
  if (ISCORail) // this is true at start
  {
    Serial.print("Started waiting for ISCO...Millis = "); Serial.println(millis()); //If ISCO is enabled, reset the watchdog while waiting for data on ISCO Serial
    do {
       //checkTexts();
    } while (iscoSerial.available() == 0 && millis() - iscoTimeout < 40000); //If there is data on the serial line, OR its been 40 seconds, continue
    iscoTimeout = millis(); //reset timer
    Serial.print("Finished waiting for ISCO...Millis = "); Serial.println(millis());
    do {  //read in data from ISCO Serial
      readIscoSerial();
    } while (iscoSerial.available() > 0);
  } else
  {
    checkTexts();
    delay(5000); //If ISCORail is disabled , wait 5 seconds every loop
  }
  Serial.println("Checking texts...");
  checkTexts(); //check for SMS commands
  //Watchdog.reset(); //ensure the system doesn't prematurely reset
  Serial.println("Made it past Texts");
  checkUserInput(); //check Serial for input commands
  getBV(); //get voltage rail readings
 


  if (currentTime - lastPost > minsToPost * 60000) //Post data every (minsToPost) minutes
  { // at the moment we post every 1 minute   
    postData();
    http.clearData();
    lastPost = millis(); //reset timer
    clearIscoSerial(); //clear isco and fona serials
  } else
  {
    Serial.print(((minsToPost * 60000) - (currentTime - lastPost)) / 1000); Serial.println(" Seconds left b4 Post");
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


//Watchdog.reset();
  if (currentTime - lastProbe >  Probetime){
   checkHydraProbes();
   lastProbe = millis();
  }


  if (millis() - lastSave > minsToSave * 60000) //Save data every (minsToSave) minutes (1 mins)
  {
    Serial.println("----------");
    Serial.println("SAVING");
    Serial.println("----------");
    if (SDcard) saveData();
    lastSave = millis();
  }


  
  if (grabSampleMode) //Display sample mode on Serial Monitor 
  {
    Serial.println("------------------Currently in GRAB Sample Mode--------------------");
  } else
  {
    Serial.println("------------------Currently in AUTO Sample Mode--------------------");
  }


  
  Serial.println("Here PRe Water level");
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
      Serial.println("------------------Currently in grab Sample Mode--------------------");
    }
  } else
  {
    Serial.println(" No Water detected in Pan");
  }

  iscoData[cdIndex] = '\0'; //null out data
  cdIndex = 0;

 checkTexts();

 if (millis() - ident_save > timer_ident * 2 *  60000) //Save ident once per 2 hours 
 //EEPROM
  {
    Serial.println("----------");
    Serial.println("Saving Ident for Viper push");
    Serial.println("----------");
    ident_save = millis();
    EEPROM.write(ident_ADR, ident);
  }
  checkTexts();
 Serial.println("Here_end_loop");
  
 ////Watchdog.reset();
}


char* parse_Intensity(String message){

int len = message.length();
int indx_semi_1 = 0;
int indx_semi_2 = 0;
int i = 0;
int count = 0;
String mess;
char* intensity;
for ( i = 0; i < len; i++)
    if (message.charAt(i) == ';')
       count++;
    if (count == 1)
      indx_semi_1 = i;
    if (count == 2){
      indx_semi_2 = i;
      i = len;
    }
 mess = message.substring(indx_semi_1, indx_semi_2);
 int leng = mess.length();
 mess.toCharArray(intensity,leng+1);
 return intensity;
}



void checkTexts() //reads SMS(s) off the Fona buffer to check for commands
{
  int ind = 0;
  String msg;
  String return_msg;
 if (!rec)
 {
   xbeeSerial.print("?"); //Sends a ? to the XBEE, prompting the Xbee to send over the Text data.
 }


 delay(10);  //Wait for Text data
 if (xbeeSerial.available()>0)
 msg = xbeeSerial.readString();
 msg = msg.trim();
 ind = msg.length();
 //Serial.print("msg is: " + msg + " length is ");
 //Serial.println(msg.length());
 if (msg.length() == 0){
  //Serial.println("No valid messages");
  return;
 }
 else {
  Serial.print("message is ");
  Serial.println(msg);
 }
 // may need to add \0 later
 if (msg[0] == '1'){
 
 float batt = getBV(); // call for battery
 String battString = String(batt);
 return_msg = battString + " mV";
 sendSMS(return_msg);
 rec = true;
 Serial.println("Found something!");
 }

else if (msg[0] == '2'){
  toggleSample();
  return_msg = "Sampling";
  delay(1000);
  sendSMS(return_msg);
  rec = true;
 }
 
else if (msg[0] =='3'){
 if (ISCORail)
      {
        ISCORail = false;  
        return_msg = "turned off ISCO";
        sendSMS(return_msg);
      } else
      {
        ISCORail = true;
        return_msg = "turned on ISCO";
        sendSMS(return_msg);
      }
      rec = true;
 }
 
else if (msg[0] =='4'){
      if (grabSampleMode)
      {
        grabSampleMode = false;
        EEPROM.write(eepromModeAddr, false);
         return_msg = "grab sample now off";
         sendSMS(return_msg);
     
      } else
      {
        grabSampleMode = true;
        EEPROM.write(eepromModeAddr, true);
        return_msg = "grab sample now on";
        sendSMS(return_msg);
      }
      rec = true;
 }
 
else if (msg.substring(0,3)=="5_"){  //This command is the one to change the Sample interval of the ISCO
   int new_time = 0;
   new_time = msg.substring(4).toInt();
   grabSampleInterval = new_time;
   EEPROM.put(eepromIntervalAddr, grabSampleInterval);
   return_msg = "new sampling time is: " + new_time; 
   sendSMS(return_msg);
   rec = true;
 }

else if (msg[0] =="6"){  // 
  minsToPost = .5;
  return_msg = "post time changed to 30 seconds";
  sendSMS(return_msg);
  rec = true;
}

else if (msg[0] == "7"){
  minsToPost = 5;
  return_msg = "post time changed to 5 minutes";
  sendSMS(return_msg);
  rec = true;
}

else if (msg.substring(0,2)=="8_"){
rec = true;
  sendSMS("No Parsivel Connected");
}

else if (msg.substring(0,2)=="9_"){
  rec = true;
 sendSMS("No RQ30 Connected");
}
else if (msg.substring(0,3)=="10_"){
  rec = true;
 sendSMS("No Sensor Connected");
}
else{
  Serial.println("No valid messages");
  return;
}

}





///////////////////////////////***********************Xbee FUNCTIONS
/*** SMS ***/

void sendSMS(String message) {
  // send an SMS!
  Serial.print("Sending: ");
  Serial.println("C"+message);
  xbeeSerial.print("C"+message);
  delay(10);
}

void massSMS(String message) {
  // send an SMS!
  xbeeSerial.print("A"+message);
}

void flushxbeeSerial() { // flush xbee Serial port
    xbeeSerial.flush();
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
          massSMS("All bottles are full. ISCO Sampler must be reset and bottles replaced");
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
    
      massSMS("All bottles are full. ISCO Sampler must be reset and bottles replaced. Will not sample");
    }
  else
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
  Serial.println("Start Clearing ISCO");
  for (int x = 0; x < 64 ; ++x)
  {
    if (iscoSerial.available())
    char a = iscoSerial.read();
    
  }
  Serial.println("Done clearing ISCO");
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
  /*
   * This function reads through the ISCO string sent over Serial to filter out the Bottle number  by
   *  finding commas to get the location of the data in the string
   */
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

float getBV()
{
float c = 0.0;
c = ina260.readBusVoltage();
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
      intWL = 1;
      levelReading = 1;
    } else
    {
      intWL = 0;
      levelReading = 0;
    }
    
    // format addX
    http.addInt("BottleNumber", getBottleNumber(), "/24");
    http.addInt("MoistureReading",droplet_read(),"rM");
    http.addFloat("BatteryVoltage", getBV(), "mV");
    http.addInt("LevelReading", intWL, "0/50");
    http.addFloat("DavisRainIntensity", inch_tips, "inches");
    http.addFloat("HProbe1Temp",temp, "degC");
    http.addFloat("HProbe1Moisture",moisture*100 ,"%"); 
    http.addFloat("HProbe1Conductivity",conductivity, "S/m");
    http.addFloat("HProbe1Permittivity",permittivity, "Dielectric Units");
    http.addFloat("HProbe2Temp",temp2, "degC");
    http.addFloat("HProbe2Moisture",moisture2*100,"%"); 
    http.addFloat("HProbe2Conductivity",conductivity2, "S/m");
    http.addFloat("HProbe2Permittivity",permittivity2, "Dielectric Units");
    http.addFloat("HProbe3Temp",temp3, "degC");
    http.addFloat("HProbe3Moisture",moisture3*100,"%"); 
    http.addFloat("HProbe3Conductivity",conductivity3, "S/m");
    http.addFloat("HProbe3Permittivity",permittivity3, "Dielectric Units");
Serial.print("data: " );Serial.print(getBottleNumber());Serial.print(",");
Serial.print(getBV());Serial.print(",");Serial.print(intWL);Serial.print(",");
Serial.print(inch_tips);Serial.print(",");Serial.print(temp);Serial.print(",");
Serial.print(moisture*100);Serial.print(",");Serial.print(conductivity);Serial.print(",");
Serial.print(permittivity);Serial.print(",");Serial.print(temp2);Serial.print(",");
Serial.print(moisture2*100);Serial.print(",");Serial.print(conductivity2);Serial.print(",");
Serial.print(permittivity2);Serial.print(",");Serial.print(temp3);Serial.print(",");
Serial.print(moisture3*100);Serial.print(",");Serial.print(conductivity3);Serial.print(",");Serial.println(permittivity3);
ident++;
    String identS = String(ident);

    
    // 2P,Data;datd;etc. is built in the HTTP Libary in the Custom Library Folder

    
    xbeeSerial.print(identS + "," + http.getData()/* + "!" */);  //place end character for data integrity.
    Serial.println(http.getData());
   // xbeeSerial.println("P"http.getData());  //backup solution
Serial.println("Done sending to VIPER");
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
  String batt = getBV();
  dtostrf(BattVoltage, 3, 2, bVChar);

  sprintf(levelChar, "%i", levelReading);

  myFile = SD.open(fileName, FILE_WRITE); //SD CARD CLASS LOOK UP FOR FEATURES
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

      myFile.print("## WET BOARD Unit ");
      myFile.print(unit);
      myFile.println(" ## ");
      myFile.println("");
      //int localHours = hour(local);
      //int utcHours = hour(utc);

      myFile.print("TIMESTAMP(EST),BottleNumber (/24),level Reading (0/1),Battery Voltage (V), rain level,Probe1_temp, Probe1_moisture, Probe1_conductivity, Probe1_permitivity,");
      myFile.println("Probe2_temp, Probe2_moisture, Probe2_conductivity, Probe2_permitivity,Probe3_temp, Probe3_moisture, Probe3_conductivity, Probe3_permitivity");
    }
    delay(10);
    buildDateTime();
    myFile.print(dateTime);
    myFile.print(",");
    myFile.print(bottleNumChar);
    myFile.print(",");
    myFile.print(levelChar);
    myFile.print(",");
    myFile.print(batt);
    myFile.print(",");
    myFile.print(rain_level);
    myFile.print(",");
    myFile.print(temp);
    myFile.print(",");
    myFile.print(moisture);
    myFile.print(",");
    myFile.print(conductivity);
    myFile.print(",");
    myFile.print(permittivity);
    myFile.print(",");
    myFile.print(temp2);
    myFile.print(",");
    myFile.print(moisture2);
    myFile.print(",");
    myFile.print(conductivity2);
    myFile.print(",");
    myFile.print(permittivity2);
    myFile.print(temp3);
    myFile.print(",");
    myFile.print(moisture3);
    myFile.print(",");
    myFile.print(conductivity3);
    myFile.print(",");
    myFile.print(permittivity3);
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



float getWaterIntensity() //parse the Water Intensity from ISCO input
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

int droplet_read(){
  if(analogRead(droplet_pin)<300) rain_level = 3;
else if(analogRead(droplet_pin)<500) rain_level = 2;
else rain_level = 1;
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
      inch_tips = tips/100;
      tips = tips/Intensity_period;
      Serial.println(tips);  //Prints out tips to monitor
  }
}
#endif



void checkHydraProbes()
//Checks all Hydraprobes and grabs 4 data points (temperature, Moisture,conductivity and permittivity
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
