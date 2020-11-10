/*
 * RS485 Master Software
 * Exercise the MAX485 Module.  This code runs on the first (Master)device.
 * Use the RS485_Slave_Test software for the second (Slave) device
 * This uses the SoftSerial.h library which comes with the Arduino IDE
 * Pins used for the soft serial port are fairly arbitrary and can be changed
 * as needed.  Just redefine them below.
 */
 #define parsivelSerial Serial1
#include <SoftwareSerial.h>
const int SSERIAL_RX_PIN = 0;  //Soft Serial Receive pin
const int SSERIAL_TX_PIN = 1;  //Soft Serial Transmit pin
const int SSERIAL_CTRL_PIN= 8;   //RS485 Direction control
//const int LED_PIN = 13;
const int RS485_TRANSMIT = HIGH;
const int RS485_RECEIVE = LOW;

// Create Soft Serial Port object and define pins to use
SoftwareSerial RS485Serial(SSERIAL_RX_PIN, SSERIAL_TX_PIN); // RX, TX


String byteReceived;
//===============================================================================
//  Initialization
//===============================================================================
void setup()
{
  delay(200);
  Serial.begin(9600);           // Start the built-in serial port
  Serial.println("Master Device");
  Serial.println("Type in upper window, press ENTER");
  
 // pinMode(LED_PIN, OUTPUT);     // Configure any output pins
  pinMode(SSERIAL_CTRL_PIN, OUTPUT);    
  delay(3);
  digitalWrite(SSERIAL_CTRL_PIN, RS485_RECEIVE);  // Put RS485 in receive mode  
  
  RS485Serial.begin(9600);   // Start the RS485 soft serial port 
}
//===============================================================================
//  Main
//===============================================================================
void loop() 
{
  if (Serial.available())         // A char(byte) has been entered in the Serial Monitor
  {
    Serial.print("Sending: ");
    byteReceived = Serial.readString();                   // Read the byte
    Serial.println(byteReceived);
    digitalWrite(SSERIAL_CTRL_PIN, RS485_TRANSMIT);  // Put RS485 in Transmit mode   
    delay(1);          
    RS485Serial.print(byteReceived);                 // Send byte to Remote Arduino
    delay(1);                                        // Wait before going back to Receive mode
    digitalWrite(SSERIAL_CTRL_PIN, RS485_RECEIVE);   // Put RS485 back into Receive mode    
  }
  
  if (RS485Serial.available())            //Data from the Slave is available
   {
     digitalWrite(SSERIAL_CTRL_PIN, RS485_RECEIVE);
     delay(10);
    byteReceived = RS485Serial.read();    // Read received byte
    Serial.print(byteReceived);           // Show on Serial Monitor
   }  
   delay(1);
}
