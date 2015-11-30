#define GSM_MODE 0
#define GPS_MODE 1
void serialMode(int mode) {
  if (mode == GSM_MODE) {
    digitalWrite(2,LOW); // Enable GSM mode
    digitalWrite(3,HIGH); // Disable GPS mode
  } else {
    digitalWrite(3,LOW); // Enable GPS mode
    digitalWrite(2,HIGH); // Disable GSM mode
  }
}

void setup() {
  pinMode(2,OUTPUT);
  pinMode(3,OUTPUT);
  pinMode(5,OUTPUT);
  serialMode(GSM_MODE);
  delay(500);
  Serial.begin(115200);
  delay(500);
}

char serBuffer[50];
#define SEARCHING 0
#define REGISTERED 1
#define REGISTERED_ROAM 2
#define REG_DENIED 3
#define REG_ERROR 4
void loop() {
  // Wait for network reg
  int reg_state = 0;
  int i;
  while (Serial.available() > 0) Serial.read();
  while (reg_state == SEARCHING) {
    while (Serial.available() > 0) Serial.read(); // flush serial buffer
    i = 0;
    Serial.println("AT+CREG?");
    while (i < 23) {
      if (Serial.available() > 0) serBuffer[i++] = Serial.read();
    }
    if (serBuffer[22] == '1') {
      reg_state = REGISTERED;
    } else if (serBuffer[22] == '5') {
      reg_state = REGISTERED_ROAM;
    } else if (serBuffer[22] == '3') {
      reg_state = REG_DENIED;
    } else if (serBuffer[22] == '0') {
      reg_state = REG_ERROR;
    }
  }
  if (reg_state == REGISTERED) digitalWrite(5, HIGH);
  while(1);
}
