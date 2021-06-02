/*****************************************************************************
Written for Teensy 3.5 or 3.2
Interfaces with a Davis Instruments Rainbucket Rainfall Counter
This is a slave for our master board which will request the intensity every 30 seconds.

*****************************************************************************/

#include <Time.h>
#include <TimeLib.h>

 int NumTips = 0; //Tip counter used by ISR
 double intensity = 0; //intensity value in mm/hr
 int lastTips = 0;
 int dailyTips = 0;
 unsigned long lastCall = millis();
int Pin = 5; //Default pin value
int Pin_Low = 2;  //Just used to drive one side of the tipping bucket reed switch low, a digitial pin is only used to make wiring easier 
int Debounce = 10;  //The minimum length of pulse which will be counted be the device, all shorter pulses are assumed to be noise
int lastDate;

static int begin = 0;
void setup() {
  Serial.begin(115200);
  Serial4.begin(115200);
  delay(5000);
  Serial.print("Rainbucket code for Davis"); //Generic begin statment for monitor
  pinMode(Pin, INPUT_PULLUP); //Setup pin for tipping bucket using internal pullup 
  pinMode(Pin_Low, OUTPUT);
  digitalWrite(Pin_Low, LOW); //Drive pin adjacent to interrupt pin low, acts as "ground" for tipping bucket
  attachInterrupt(digitalPinToInterrupt(Pin), Tip, CHANGE); //Setup an interrupt for the tipping bucket pin, with Tip as the ISR, which will activate on every edge
 
}
int counter =0;
void loop() {

 ++counter;
 if(day(now())>lastDate)
 {
  dailyTips = 0;
  lastDate = day(now());
  Serial.println("new date");
 }
  delay(50);
  Serial.print(".");
  if (counter>70)
  {
    counter=0;
    Serial.println(".");
  }
if (Serial4.available())
{
  Serial.println("Data");
  if (Serial4.read() == 'R')
  {
    Serial.println("Data request received - Sending");
    sendRainData();
  }

}

}

void sendRainData()
{
  unsigned long thisCall = millis();
  double secsElapsed = (thisCall-lastCall)/1000;

  double inchesSinceLastCall = (NumTips*0.01);
  intensity = (inchesSinceLastCall/secsElapsed)*25.4*3600;

  
  dailyTips = dailyTips + NumTips;

  Serial4.print(intensity);
  Serial4.print(",");
  Serial4.print(NumTips); 
  Serial4.print(",");
  Serial4.println(dailyTips); 
  
  NumTips = 0;
  
}

void Tip() {  //ISR for tipping events
  static long StartPulse = 0; //Used as variable to measure time between interrupt edges
  Serial.println("In ISR");
  if(((millis() - StartPulse) > Debounce) && digitalRead(Pin)) { //Check if the last edge was more than 1 debounce time period ago, and that the edge measured is rising
    NumTips++; //If true, increment the tip counter
    Serial.println("tip");
  }
  StartPulse = millis(); //Keep a record of the last edge time
}


  
 
