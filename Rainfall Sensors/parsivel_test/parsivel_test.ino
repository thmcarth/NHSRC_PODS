
/////////////////////////Parsivel data 
String parsivel_data;
char* parsivel_intensity;
/////////////////////////
void setup() {
  // put your setup code here, to run once:
Serial2.begin(19200);
Serial.begin(9600);
delay(5000);
//setup_parsivel();
}

void loop() {
  delay(1000);
  //read_parsivel();
  if (Serial2.available()>0)
  Serial.println(Serial2.readString());
  else 
  Serial.println("sad");
}

void setup_parsivel() { // Tells the Parsivel through serial message how we want to get the telegram data from it
  //refer to OneDrive in Parsivel2-->Terminal Commands Pdf.
  String interval = "10"; // This is how often we want to receive data from the parsivel
  
  //String request_data = "CS/M/S/%01,/%02,/%60,/%34,/%18,/%93/r/n/%61/r/\r";// this asks for  intensity, rain accumulated, particles detected, 
  // kinetic energy, and raw data (in this order)
  String request_data = "CS/M/S/%01,%02,%60,%34,%18,%93/r/n";
  String interval_send = "CS/I/10";
  String enable_msg = "CS/M/M/1";
  Serial2.print(request_data);
  delay(50);
  Serial.println(Serial2.readString());
  Serial.println(Serial2.readString());
  Serial2.print(interval_send);
  Serial.println(Serial2.readString());
  
  Serial.println("Parsivel Setup");
}

void read_parsivel(){
  Serial.println("Start Reading");
  String message = "";
  int i = 0;
  //Serial2.print("CS/R/%13\r");
  delay(500);
  if (Serial2.available()>0){
   message = Serial2.readString();
  }
    Serial.print("Done Reading, msg is: ");
  Serial.println(message);
  parsivel_data = message;
  parsivel_intensity = parse_Intensity(message);
  Serial.print("Done Reading, value is: ");
  Serial.println(parsivel_intensity);
}

char* parse_Intensity(String message){
Serial.print("Message is");
Serial.println(message);
int len = message.length();
int indx_semi_1 = 0;
int indx_semi_2 = 0;
int i = 0;
int count = 0;
String mess;
char* intensity;
if (len >= 2){  // Intensity value is at least 8 digits.  If the message isnt this long, there's nothing to look at.
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
else
return "Empty";
}
void serialEvent2() {
  Serial.println(Serial2.readString());
}
