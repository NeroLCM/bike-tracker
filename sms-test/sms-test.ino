void setup() {
  Serial.begin(115200);
}

void loop() {
  char data[5];
  /*Serial.println("AT");
  delay(1000);  
  Serial.println("AT");
  delay(1000);*/
  // Wait for network registration
  // Should use AT+CREG to check and expect 0,1
  bool registered = false;
  /*while (!registered) {
    Serial.println("AT+CREG");
    delay(500);
    int i = 0;
    while (Serial.available() > 0) {
      data[i] = Serial.read();
      Serial.print(data[i]);
    }
    Serial.println("");
    if (data[3] == '1') registered = true;
  }*/
  // Send SMS
  Serial.println("AT+CMGF=1");
  delay(1000);
  Serial.println("AT+CMGS=\"5103983664\"");//Change the receiver phone number
  delay(1000);
  Serial.print("Arduino SMS Test.");//the message you want to send
  delay(1000);
  Serial.write(26);
  while(1) ;
}
