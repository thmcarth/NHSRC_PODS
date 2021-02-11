void setup() {
Serial1.begin(115200);
Serial.begin(9600);
}

String dat= "234<PBottleNumber;1;/24;Green;MoistureReading;3;rM;Green;BatteryVoltage;12115.000000;mV;Green;LevelReading;1;0/50;Green;DavisRainIntensity;259.000000;inches;Green;HProbe1Temp;0.000000;degC;Green;HProbe1Moisture;0.000000;%;Green;HProbe1Conductivity;0.000000;S/m;Green;HProbe1Permittivity;0.000000;Dielectric Units;Green;HProbe2Temp;0.000000;degC;Green;HProbe2Moisture;0.000000;%;Green;HProbe2Conductivity;0.000000;S/m;Green;HProbe2Permittivity;0.000000;Dielectric Units;Green;HProbe3Temp;0.000000;degC;Green;HProbe3Moisture;0.000000;%;Green;HProbe3Conductivity;0.000000;S/m;Green;HProbe3Permittivity;0.000000;Dielectric Units;Green!";

void loop() {
  if (Serial.available()){
   char a = Serial.read();
 if (a == 's'){
  int len = dat.length();
  int d = len/3;
  int tot = d*3;
 
  String one = dat.substring(0,d);
  String two = dat.substring(d,d*2);
  String three = dat.substring(d*2);
  
  Serial.println(one);
  Serial.println(two);
  Serial.println(three);

  Serial1.print(one);
  delay(7);
  Serial1.print(two);
  delay(8);
  Serial1.print(three);
//Serial1.print(dat);
  Serial.println("Sent");
 }
if (a == 't'){


Serial1.print("?!");
  Serial.println("Sent ?!");
 }

    
  }

}
