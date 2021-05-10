/*
This code measures liquid flow rate and cumulative volume using a Digiten Water Flow Sensor Model: FL-1608
connected to a Teensy 3.5 with the signal line attached to digital pin 3. 
The liquid flow rate portion of the code was adapted from DIYhacking.com Liquid flow rate sensor by Arvind Sanjeev
by addint time stamp and sd card datalogging features.
 */

#include <TimeLib.h>
#include <SD.h>

byte statusLed    = 11;

byte sensorInterrupt = 3;  // 0 = digital pin 3
byte sensorPin       = 3;

// The hall-effect flow sensor outputs approximately 12 pulses per second per
// litre/minute of flow.
float calibrationFactor = 12;

volatile byte pulseCount;  

float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;

unsigned long oldTime;

//datalogging
const int chipSelect = BUILTIN_SDCARD;

void setup()
{

  // set the Time library to use Teensy's RTC to keep time
  setSyncProvider(getTeensy3Time);
  
  // Initialize a serial connection for reporting values to the host
  Serial.begin(9600);

  while (!Serial);  // Wait for Arduino Serial Monitor to open
  delay(100);
  if (timeStatus()!= timeSet) {
    Serial.println("Unable to sync with the RTC");
  } else {
    Serial.println("RTC has set the system time");
  }

  // initialize the SD card
  Serial.print("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(BUILTIN_SDCARD, OUTPUT);
  
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");
   
  // Set up the status LED line as an output
  pinMode(statusLed, OUTPUT);
  digitalWrite(statusLed, HIGH);  // If you have an active-low LED attached
  
  pinMode(sensorPin, INPUT);
  digitalWrite(sensorPin, HIGH);

  pulseCount        = 0;
  flowRate          = 0.0;
  flowMilliLitres   = 0;
  totalMilliLitres  = 0;
  oldTime           = 0;

  // The Hall-effect sensor is connected to pin 3 which uses interrupt 0.
  // Configured to trigger on a FALLING state change (transition from HIGH
  // state to LOW state)
  attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
  
  File dataFile = SD.open("datalog.txt", FILE_WRITE);
  dataFile.println("L/min,mL,L,h:m:s,month,day,year");
  dataFile.close();
}


/**
 * Main program loop
 */


void loop()
{
    if (Serial.available()) {
    time_t t = processSyncMessage();
    if (t != 0) {
      Teensy3Clock.set(t); // set the RTC
      setTime(t);
    }
  }
  digitalClockDisplay();  
  delay(1000);

   if((millis() - oldTime) > 1000)    // Only process counters once per second
  { 
    // Disable the interrupt while calculating flow rate and sending the value to
    // the host
    detachInterrupt(sensorInterrupt);
        
    // Because this loop may not complete in exactly 1 second intervals we calculate
    // the number of milliseconds that have passed since the last execution and use
    // that to scale the output. We also apply the calibrationFactor to scale the output
    // based on the number of pulses per second per units of measure (litres/minute in
    // this case) coming from the sensor.
    flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / calibrationFactor;
    
    // Note the time this processing pass was executed. Note that because we've
    // disabled interrupts the millis() function won't actually be incrementing right
    // at this point, but it will still return the value it was set to just before
    // interrupts went away.
    oldTime = millis();
    
    // Divide the flow rate in litres/minute by 60 to determine how many litres have
    // passed through the sensor in this 1 second interval, then multiply by 1000 to
    // convert to millilitres.
    flowMilliLitres = (flowRate / 60) * 1000;
    
    // Add the millilitres passed in this second to the cumulative total
    totalMilliLitres += flowMilliLitres;
      
    //unsigned int frac;

    File dataFile = SD.open("datalog.txt", FILE_WRITE);
    
    //log data to sd card and print to the serial monitor 
    //the flow rate for this second in L/min and 
    //the cumulative total of litres flowed since starting (in mL and L)  
    dataFile.print(int(flowRate));  // Print the integer part of the variable
    dataFile.print(",");
    dataFile.print(totalMilliLitres);
    dataFile.print(","); 
    dataFile.print(totalMilliLitres/1000);
    dataFile.print(",");
    dataFile.close();
    Serial.print(int(flowRate));  // Print the integer part of the variable
    Serial.print(",");
    Serial.print(totalMilliLitres);
    Serial.print(","); 
    Serial.print(totalMilliLitres/1000);
    Serial.print(",");
    

    // Reset the pulse counter so we can start incrementing again
    pulseCount = 0;
    
    // Enable the interrupt again now that we've finished sending output
    attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
  }
}

void digitalClockDisplay() {
  File dataFile = SD.open("datalog.txt", FILE_WRITE);
   // digital clock display of the time
  dataFile.print(hour());
  dataFile.print(":");
  dataFile.print(minute());
  dataFile.print(":");
  dataFile.print(second());
  dataFile.print(", ");
  dataFile.print(month());
  dataFile.print(", ");
  dataFile.print(day());
  dataFile.print(", ");
  dataFile.print(year()); 
  dataFile.println();
  dataFile.close();
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(", ");
  Serial.print(month());
  Serial.print(", ");
  Serial.print(day());
  Serial.print(", ");
  Serial.print(year()); 
  Serial.println();
}

/*
Insterrupt Service Routine
 */
void pulseCounter()
{
  // Increment the pulse counter
  pulseCount++;
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

  if(Serial.find(TIME_HEADER)) {
     pctime = Serial.parseInt();
     return pctime;
     if( pctime < DEFAULT_TIME) { // check the value is a valid time (greater than Jan 1 2013)
       pctime = 0L; // return 0 to indicate that the time is not valid
     }
  }
  return pctime;
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}
