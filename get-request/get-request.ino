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

int8_t sendCommand(char* command, char* expected_answer1, unsigned int timeout){
    uint8_t x=0,  answer=0;
    char response[100];
    unsigned long previous;
    memset(response, '\0', 100);    // Initialize the string
    delay(100);

    while(Serial.available() > 0) Serial.read();    // Clean the input buffer
    Serial.println(command);    // Send the AT command 
    x = 0;
    previous = millis();

    // this loop waits for the answer
    do {
        if (Serial.available() != 0){    
            response[x] = Serial.read();
            x++;
            // check if the desired answer is in the response of the module
            if (strstr(response, expected_answer1) != NULL) {
                answer = 1;
            }
        }
        // Waits for the asnwer with time out
    } while((answer == 0) && ((millis() - previous) < timeout));    
    return answer;
}

void setup() {
  pinMode(2,OUTPUT);
  pinMode(3,OUTPUT);
  pinMode(5,OUTPUT);
  pinMode(6,OUTPUT);
  pinMode(7,OUTPUT);
  serialMode(GSM_MODE);
  delay(500);
  Serial.begin(115200);
  delay(500);
}

void loop() {
  // put your main code here, to run repeatedly:
  int answer = 0;
  char buffer_str[127];
  char content[512];
  char url[] = "lcm.im/ee149/index.php?token=598234c470cd87148cd26090f83ac4e6";
  answer = sendCommand("AT+HTTPINIT", "OK", 2000);
  if (answer != 1) return; // init error
  answer = sendCommand("AT+HTTPPARA=\"CID\",1", "OK", 5000); // CID = 1 --> GET request
  if (answer != 1) return; // action settings error
  snprintf(buffer_str, sizeof(buffer_str), "AT+HTTPPARA=\"URL\",\"%s\"", url);
  answer = sendCommand(buffer_str, "OK", 5000);
  if (answer != 1) return; // url set error
  answer = sendCommand("AT+HTTPACTION=0", "+HTTPACTION:0,200", 10000);
  if (answer != 1) return; // response code error
  // The server's response is never longer than 500 bytes. Therefore, it's safe to read only 500 bytes of data.
  while (Serial.available() > 0) Serial.read();
  Serial.println("AT+HTTPREAD=0,500");
  delay(1000);
  int i = 0;
  while (Serial.available() > 0) {
    content[i++] = Serial.read();
  }

  // for debug
  char[] expected_str = "Welcome, Arduino!";
  int action = HIGH;
  for (i = 0; i < sizeof(expected_str); i++) {
    if (content[i] != expected_str[i]) {
       action = LOW;
       break;
    }
  }
  digitalWrite(5, action);
  while (1);
}
