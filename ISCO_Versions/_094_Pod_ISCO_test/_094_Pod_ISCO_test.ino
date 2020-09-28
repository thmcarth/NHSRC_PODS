#include <HydraProbe.h>
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
//Version TEST - TEST EACH AND EVERY SENSOR ON THE PROTOTYPE BOARD



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
int eepromfirst_upload = 2;
int is_first = 0;
#define FIRST 0
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

/////////////////////////Parsivel data 
String parsivel_data;
char* parsivel_intensity;
/////////////////////////

//DAVIS RAINBUCKET
#define DAVIS 1
///////////////I2C Comms
#include <Wire.h>
int Intensity_period = 30; // Intensity period and how often we want to ask device for rainfall intensity.
unsigned long UpdateRate = 4000; //Number of ms between serial prints, 4 seconds by default
uint8_t ADR = 0x08; //Address of slave device, 0x08 by default
long rain_period = 30000;
long previousMillis = 0; 
int rain_level = 0; //0 is no rain, 1 is little, 2 is some, 3 is lots of rain 
int droplet_pin = 10;
//INA260 comms Placeholder
//////////////END I2C Inits


#define HydraSerial Serial3
//String moisture;
float temp, moisture, conductivity,permittivity;
float temp2, moisture2, conductivity2,permittivity2;
float temp3, moisture3, conductivity3,permittivity3;
HydraProbe moistureSensor;  //define data Pin in Header (library .h file)
HydraProbe moistureSensor2;
HydraProbe moistureSensor3;
bool HPRail = true;
unsigned long lastProbe = millis();
int Probetime = 60000;
//
//FONA HTTP
HTTPS_VIPER http = HTTPS_VIPER(); 
char senderNum[30];    //holds number of last number to text SIM
char replybuffer[255]; //holds Fonas text reply
char * replybuffer_command; //holds Fonas command part
char * replybuffer_interval; //holds Fonas interval part
HardwareSerial *xbeeSerial = &Serial5;
HardwareSerial *parsivelSerial = &Serial1;
int commandNum = -1; //integer value for which text command has been sent
unsigned long lastPost = millis();
float minsToPost = 5.0;
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
int bottle_limit = 4;
int bottle_total = 0;
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


///////////////////////////RELAYS
bool RQ30 = true;
//Soil moisture data is on pin 30
#define HPRelay 27 //Outpin PIN to control HydraProbe 12V Rail
#define RQ30Relay 26 //Output PIN to control RQ30 12V Rail
//unsigned long fonaTimer = millis(); //timer to reset Fona
unsigned long lastRQ = millis();
unsigned long RQTime = 60000*5;
//////////////////////////RELAYS
Adafruit_INA260 ina260 = Adafruit_INA260();

void setup() {
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
  delay(500);
  
  pinMode(BattRail_VIN, INPUT);
  digitalWrite(HPRelay, HIGH); //turn on HydraProbe 12V Rail
  delay(500);
  moistureSensor.debugOn(); //Can turn debug on or off to see verbose output (OR NOT)
  moistureSensor.begin(1);
  Watchdog.reset();

  
  #if FIRST  //change FIRST to 1 when uploading and running first time
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
  Serial.print("Interval is ");Serial.print(grabSampleInterval);Serial.println(" minutes");
  xbeeSerial->begin(9600); //Startup SMS and Cell client
  
  pinMode(IscoSamplePin, OUTPUT);  // Setup sample pin output
  digitalWrite(IscoSamplePin, LOW);
  Serial.begin(9600);
  iscoSerial.begin(9600); //Setup comms with ISCO
  Watchdog.reset();
  setup_parsivel();
}






void loop() {
  delay(1000); 
  if (Serial.available()>0){
  char comm = Serial.read();
  Serial.print("command is: ");
  Serial.println((int)comm);
  if (comm == 'a'){ //BV
    Serial.println("Seeking out Battery Voltage");
  float c = getBV();
  delay (1000);
  Serial.println(c);
  }

else if (comm == 'b'){ //parsivel 
  
read_parsivel();
Serial.print("intensity is: " );
Serial.println(parsivel_intensity);
if (parsivel_data.length()>0)
Serial.println("data retrieved successfully");
else 
Serial.println("data not retrieved");
}

else if (comm == 'c'){ //hydraprobes
  checkHydraProbes();
}

else if (comm == 'd'){ // water level sensor
  Serial.println("Water level is " + getWaterLevel());
 }
else if (comm == 'e'){ //Davis Rain boi
  I2C_rain();
}

else if ( comm == 'f'){ //Xbee stuff
  testPost();
}
else if (comm =='g'){ //moisture sensor
  droplet_read();
}
else if (comm =='h'){
  Serial.println("h command send");  // test line to make sure program is running
}
else if (comm =='i'){
  changeAddress();
}
else if (comm =='j'){
  Serial.println(moistureSensor.getAddress());
}
else {Serial.println("wrong command or leftover serial text");}
  }
  else {
    //Serial.println ("no command seen");
  }
Serial.flush();
}


void testPost() //post Data to VIPER
{
   /* int intWL;
    if (getWaterLevel()) //Transpose the water level to 50 (wet) or 0 (dry) for viewing on VIPER
    {
      intWL = 50;
      levelReading = 50;
    } else
    {
      intWL = 0;
      levelReading = 0;
    }
    */
    //http.addInt("Bottle Number", getBottleNumber(), "/24");
    //http.addFloat("Battery Voltage", getBV(), "V");
    http.addInt("Level Reading", 50, " 0/50");
    //http.addString("Parsivel Intensity",parsivel_intensity, "mm/h");
    xbeeSerial->println(http.getData());
   // xbeeSerial->println("P"http.getData());  //backup solution
   Serial.println("Sent");

}



void setup_parsivel() { // Tells the Parsivel through serial message how we want to get the telegram data from it
  //refer to OneDrive in Parsivel2-->Terminal Commands Pdf.
  String interval = "60"; // This is how often we want to receive data from the parsivel
  
  String request_data = "CS/M/S/%19,/%01,/%02,/%60,/%34,/%18,/%93/r/n";// this asks for date/time, intensity, rain accumulated, particles detected, 
  // kinetic energy, and raw data (in this order)
  String interval_send = "CS/I/"+interval;
  String enable_msg = "CS/M/M/1";
  parsivelSerial->print(request_data);
  delay(1000);
  parsivelSerial->print(interval_send);
  delay(1000);
  parsivelSerial->print(enable_msg);
  delay(500);
}

void read_parsivel(){
  String message = "";
  int i = 0;
  while (parsivelSerial->available()){
   message = ""+ message + parsivelSerial->read()+ "";
  }
  parsivel_data = message;
  parsivel_intensity = parse_Intensity(message);
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



/*void checkHydraProbes(){
// Serial Println style commands here: over the Serial Port we will need: commands /r/n
HydraSerial.println("AAATR");
//char reading[100]; 
//int i = 0;
String values="";
while (HydraSerial.available()){
 values += HydraSerial.readString();
}
//moisture = values;
}
*/


void checkTexts() //reads SMS(s) off the Fona buffer to check for commands
{
  int ind = 0;
  String msg;
  String return_msg;
 while (xbeeSerial->available()){
 msg[ind++]=xbeeSerial->read();
 }
 msg.trim();
 // may need to add \0 later
 if (msg == "C1"){
 
 String batt = getBV(); // call for battery
 return_msg = batt;
 sendSMS(return_msg);
 }

else if (msg =="C2"){
  Sample();
  return_msg = "Sampling...";
  sendSMS(return_msg);
 }
 
else if (msg =="C3"){
 if (ISCORail)
      {
        ISCORail = false;  
        return_msg = "turned off";
        sendSMS(return_msg);
      } else
      {
        ISCORail = true;
        return_msg = "turned on";
        sendSMS(return_msg);
      }
 }
 
else if (msg =="C4"){
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
 }
 
else if (msg.substring(0,3)=="C5_"){  //This command is the one to change the Sample interval of the ISCO
   int new_time = 0;
   new_time = msg.substring(4).toInt();
   grabSampleInterval = new_time;
   EEPROM.put(eepromIntervalAddr, grabSampleInterval);
   return_msg = "new sampling time is: " + new_time; 
   sendSMS(return_msg);
 }

else if (msg =="C6"){  // 
  minsToPost = .5;
  return_msg = "post time changed to 30 seconds";
  sendSMS(return_msg);
}

else if (msg == "C7"){
  minsToPost = 5;
  return_msg = "post time changed to 5 minutes";
  sendSMS(return_msg);
}

else if (msg.substring(0,3)=="C8_"){
  rain_period = msg.substring(4).toInt();
  return_msg = "new rainfall read time is: " + rain_period;
  sendSMS(return_msg);
}

else if (msg.substring(0,3)=="C9_"){
 RQTime = msg.substring(4).toInt();
 return_msg = "new RQ read time is: " + RQTime;
 sendSMS(return_msg);
}
else if (msg.substring(0,3)=="C10_"){

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

bool sendSMS(String message) {
  // send an SMS!
 
 xbeeSerial->print("C");
  xbeeSerial->println(message);
}

bool massSMS(String message) {
  // send SMS to all users!
 xbeeSerial->print("A");
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
  {
    Serial.println("Start sampler");
    textTimer = millis();
    timerOn = true;
      bottle_total++;
      Serial.println(bottle_total);
    {
      botNum = getBottleNumber() + 1;
      if (botNum > 24)
      {
        botNum = 1;
        bottle_total = 1;

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


String getBVs()
{
  /*
   * int INA = B100;00000; //connect
     uint8_t INA_I = 0x01;
     uint8_t INA_V = 0x02;
   */
String c = "0";
  return c;
  
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
      intWL = 50;
      levelReading = 50;
    } else
    {
      intWL = 0;
      levelReading = 0;
    }
    http.addInt("Bottle Number", getBottleNumber(), "/24");
    http.addFloat("Battery Voltage", getBV(), "V");
    http.addInt("Level Reading", intWL, " 0/50");
    http.addString("Parsivel Intensity",parsivel_intensity, "mm/h");
    xbeeSerial->println(http.getData());
    //xbeeSerial->println(http.getData());  //backup solution

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
    myFile.print(",");
    myFile.print(parsivel_data);
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

#if DAVIS 
void droplet_read(){
  if(analogRead(droplet_pin)<300) {
    rain_level = 3;
    Serial.println("Raining hard");
  
  }
else if(analogRead(droplet_pin)<500){
  rain_level = 2;
  Serial.println("Raining medium");
}
else{
  rain_level = 1;
  Serial.println("Raining none");
}
}
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

void changeAddress(){
  moistureSensor.changeAddress(0);
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


  /*if (moistureSensor2.getHPStatus())
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
  }*/
}
