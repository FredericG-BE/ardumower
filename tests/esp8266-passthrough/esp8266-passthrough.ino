void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial1.begin(115200);
  
  Serial.print("esp8266 passthrough\n");
}

void loop() {
  if (Serial1.available()>0){
    Serial.write(Serial1.read());
  }
  if (Serial.available()>0){
    Serial1.write(Serial.read());
    //Serial1.write('A');
  }

}
