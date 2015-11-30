#include <stdlib.h>
char UTC[6] = {'0', '0', '0', '0', '0', '0'};
float lon = 0.0, lat = 0.0;
char lond = 0, latd = 0; // direction of the lon and lat
char fix = '0'; // fix data


char gps_align() {
  // This is used to align GPS return values 
  char i = 0;
  char value[6] = {
    '$','G','P','G','G','A'
  };
  char val[6] = {
    '0','0','0','0','0','0'
  };

  while (1) {
    if (Serial.available()) {
      val[i] = Serial.read(); //get the data from the serial interface
      
      if(val[i] == value[i]) {    
        i++;
        if (i==6) {
          i=0;
          return 1; // aligned
        }
      }
      else i=0;
    }
  } 
}
 
void comma(char num) {   
  char val;
  char count = 0; // count the number of ','
 
  while(1) {
    if (Serial.available()) {
      val = Serial.read();
      if(val == ',') count++;
    }
    if (count == num) return;
  }
}

void updateUTC() {
  char gps_buffer[9];
  int i = 0;
  gps_align();
  comma(1);
  while (1) {
    if (Serial.available()) {
      gps_buffer[i] = Serial.read();
      i++;
    }
    if (i == 9) {
      for (i = 0; i < 6; i++) {
        UTC[i] = gps_buffer[i];
      }
      return;
    }
  }
}

void updateLat() {
  char gps_buffer[11];
  char tchar[2];
  int i = 0;
  gps_align();
  comma(2);
  while (1) {
    if (Serial.available()) {
      gps_buffer[i] = Serial.read();
      i++;
    }
    if (i == 11) {
      // ddmm.mmmmmm
      tchar[0] = gps_buffer[0];
      tchar[1] = gps_buffer[1];
      int lat_deg = atoi(tchar);
      float lat_min = atof(gps_buffer);
      lat = lat_deg * 1.0 + (lat_min - lat_deg * 100.0) / 60.0;
      return;
    }
  }  
}

void updateLatd() {
  gps_align();
  comma(3);
  while (1) {
    if (Serial.available()) {
      latd = Serial.read();
      return;
    }
  }
}

void updateLon() {
  char gps_buffer[11];
  char tchar[3];
  int i = 0;
  gps_align();
  comma(4);
  while (1) {
    if (Serial.available()) {
      gps_buffer[i] = Serial.read();
      i++;
    }
    if (i == 11) {
      // dddmm.mmmmm
      tchar[0] = gps_buffer[0];
      tchar[1] = gps_buffer[1];
      tchar[2] = gps_buffer[2];
      int lon_deg = atoi(tchar);
      float lon_min = atof(gps_buffer);
      lon = lon_deg * 1.0 + (lon_min - lon_deg * 100.0) / 60.0;
      return;
    }
  }  
}

void updateLond() {
  gps_align();
  comma(5);
  while (1) {
    if (Serial.available()) {
      lond = Serial.read();
      return;
    }
  }
}

void updateFix() {
  gps_align();
  comma(7);
  while (1) {
    if (Serial.available()) {
      fix = Serial.read();
      return;
    }
  }
}

void updateAll() {
  updateFix();
  updateUTC();
  updateLat();
  updateLatd();
  updateLon();
  updateLond();
}

void serial_print_data() {
  Serial.println("GPS data: ");

  Serial.print("Fix: ");
  Serial.write(fix);
  Serial.println(" ");
  
  // UTC time
  Serial.print("Time: "); Serial.write(UTC[0]); Serial.write(UTC[1]);
  Serial.print(":"); Serial.write(UTC[2]); Serial.write(UTC[3]);
  Serial.print(":"); Serial.write(UTC[4]); Serial.write(UTC[5]); Serial.println(" ");

  // Location
  Serial.print("Location: "); Serial.write(latd); Serial.print(" "); Serial.print(lat, 6);
  Serial.print(", "); Serial.write(lond); Serial.print(" "); Serial.print(lon, 6);
  Serial.println(" ");
  Serial.println(" ");
}

void setup() {
  //The digital driver pins for the GSM and GPS mode
  pinMode(2,OUTPUT);
  pinMode(3,OUTPUT);
  pinMode(5,OUTPUT);
  digitalWrite(5,HIGH);
  delay(1000);
  digitalWrite(5,LOW);
 
  digitalWrite(2,LOW); // Enable GSM mode
  digitalWrite(3,HIGH); // Disable GPS mode
  delay(1000);
  Serial.begin(115200);
  delay(1000);
  //turn on GPS power supply
  Serial.println("AT+CGPSPWR=1");
  delay(1000);
  //reset GPS in autonomy mode
  Serial.println("AT+CGPSRST=1");
  delay(1000);
 
  digitalWrite(3,LOW); // Enable GPS mode
  digitalWrite(2,HIGH); // Disable GSM mode
  delay(1000);
}

int fix_count = 0;
bool sent = false;
void loop() {
  updateAll();
  serial_print_data();

  if (fix == '6' || fix == '7' || fix == '8') {
    fix_count++;
  } else {
    fix_count = 0;
  }

  if (fix_count > 1 && sent == false) {
    // fixed for some time now
    Serial.println("Sending text to 5103983053");
    delay(500);
    // send a SMS to my phone!
    digitalWrite(2,LOW); // Enable GSM mode
    digitalWrite(3,HIGH); // Disable GPS mode
    delay(750);
    Serial.println("AT+CMGF=1");
    delay(500);
    Serial.println("AT+CMGS=\"5103983053\"");
    delay(500);
    serial_print_data();
    delay(500);
    Serial.write(26);
    delay(500);
    sent = true;
    digitalWrite(3,LOW); // Enable GPS mode
    digitalWrite(2,HIGH); // Disable GSM mode
  }
}
