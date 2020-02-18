int digital_line = 15;
int analog_line = 14;
int condition = 0;
void setup() {
  // put your setup code here, to run once:
pinMode(15,OUTPUT);
Serial.begin(9600);

}

void loop() {
  // put your main code here, to run repeatedly:
if (analogRead(analog_line) <300)
condition = 3;
if (analogRead(analog_line) <500)
condition = 2;
else
condition = 1;

delay(250); // remove this if the loop becomes a function in a separate program
}
