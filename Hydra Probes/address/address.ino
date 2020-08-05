#include <SDI12.h>
//#include <SDISerial.h>
#include <SoftwareSerial.h>
#include <OneWire.h>
int Hydra = 30;
#define HPRelay 27 //Outpin PIN to control HydraProbe 12V Rail
String change_addr = "Ab!";
String serial_ID = "0I!";
String Reading_0 = "0M!";
SoftwareSerial HSerial(12, 30); // RX, TX
SDI12 Hydra_0(&HSerial, '1', true);
void setup() {
HSerial.begin(1200);
Serial.begin(9800);
pinMode(HPRelay, OUTPUT); //Set up HydraProbe relay
delay(50);
digitalWrite(HPRelay, HIGH); //turn on HydraProbe 12V Rail
delay(15);
}
int test = 0;
void loop() {
  char data[75];
  char debug[10];
  bool error;
  test++;
//  while(Serial.available() ==0);
//  String commands = Serial.readString();
  Serial.println();

  
  memset( data, 0 ,75 );
  Hydra_0.transparent("0I!", data);
  Serial.println(data);
  /*
  memset( data, 0 ,75 );
  Hydra_0.transparent( "", data );
  Serial.print(data);
 
  memset( data, 0, 75 );
  memset( debug, 0, 10 );
  
  error = Hydra_0.verification( debug );
  if ( !error ) Serial.print( debug );
  
  error = Hydra_0.returnMeasurement( data, 0 );
  if ( !error ) Serial.print( data );
  */
   delay(1000);
}
String get_SDI(String sending){
  //pinMode(Hydra, OUTPUT);
  delay(1);
  HSerial.print(sending);
  String answer = receive_SDI();
  return answer;
 

}

String receive_SDI(){
  //pinMode(Hydra, INPUT);
  delay(5);
 String answer= HSerial.readString();
 return answer;
}
