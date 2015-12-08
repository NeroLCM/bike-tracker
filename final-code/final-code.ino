#include <stdlib.h>
#include <EEPROM.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

////////////////// code for Arduino sleep mode ///////////////////
/* 
 *  In sleep mode, the board wakes up every 8 secs. This is the maximum
 *  time for the internal watchdog timer. Therefore we want to sleep 225
 *  times (~30 min) before we really wake up.
 */
#define SLEEP_TIMES 2
volatile int wd_count=0;

ISR(WDT_vect){
  wd_count++;
}

void enter_power_saving_mode() {
  setup_sleep_mode_param();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  wd_count = 0;
  sleep_enable();
  sleep_mode(); // enters sleep mode here
  while (wd_count < SLEEP_TIMES) {
    sleep_mode();
  }    
  sleep_disable();
  wdt_disable();
  /* Re-enable the peripherals. */
  power_all_enable();
}

void setup_sleep_mode_param() {
  MCUSR &= ~(1<<WDRF);
  WDTCSR |= (1<<WDCE) | (1<<WDE);
  WDTCSR = 1<<WDP0 | 1<<WDP3;
  WDTCSR |= _BV(WDIE); // Enables watchdog interrupt
}
/////////////// END - code for Arduino sleep mode ////////////////

////////////// code for serial command interface //////////////
void flush_serial_input_buffer() {
  while (Serial.available() > 0) Serial.read();
}

int send_at_command_and_select(char* command, char* opt1, char* opt2, int timeout) {
  char i = 0;
  char res[150];
  unsigned long time_stamp;
  memset(res, '\0', 150);
  delay(100);
  while (Serial.available() > 0) Serial.read(); // flush the input buffer
  time_stamp = millis();
  while (millis() - time_stamp < timeout) {
    if (Serial.available() > 0) {
      res[i++] = Serial.read();
      if (strstr(res, opt1) != NULL) {
        return 1;
      } else if (strstr(res, opt2) != NULL) {
        return 2;
      }
    }
  }
  return 0;
}

int send_at_command_and_validate(char* command, char* expected, int timeout) {
  return send_at_command_and_select(command, expected, expected, timeout);
}

////////////// END code for serial command interface //////////////

///////////// code for GPS comm /////////////
char UTC[6] = {'0', '0', '0', '0', '0', '0'};
float lon = 0.0, lat = 0.0;
char lond = 0, latd = 0; // direction of the lon and lat
char fix = '0'; // fix data

// aligns the input pointer to GPS data
char gps_align() {
  // This is used to align GPS return values 
  char i = 0;
  char value[6] = {'$','G','P','G','G','A'};
  char val[6] = {'0','0','0','0','0','0'};
  while (1) {
    if (Serial.available()) {
      val[i] = Serial.read(); //get the data from the serial interface
      if(val[i] == value[i]) {    
        i++;
        if (i == 6) {
          return 1; // aligned
        }
      }
      else i = 0;
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

void formatGPSData() {
  if (latd != 'N') {
    lat = -lat;
  }
  if (lond != 'E') {
    lon = -lon;
  }
}

void updateAllGPSData() {
  updateFix();
  updateUTC();
  updateLat();
  updateLatd();
  updateLon();
  updateLond();
  formatGPSData();
}

///////////// END code for GPS comm /////////////

//////////// code for GSM/GPRS registration ////////////
int wait_for_GSM_registration(unsigned long timeout) {
  unsigned long time_stamp = millis() + timeout;
  while (millis() < time_stamp && 
    (send_at_command_and_select("AT+CREG?", "+CREG: 0,1", "+CREG: 0,5", 3000) == 0));
  if (send_at_command_and_select("AT+CREG?", "+CREG: 0,1", "+CREG: 0,5", 3000) != 0) {
    return 0;
  }
  return 1;
}

void setup_GPRS_param() {
  send_at_command_and_validate("AT+SAPBR=3,1,\"Contype\",\"GPRS\"", "OK", 3000);
  send_at_command_and_validate("AT+SAPBR=3,1,\"APN\",\"phone\"", "OK", 3000);
  while (send_at_command_and_validate("AT+SAPBR=1,1", "OK", 20000) == 0) delay(4000);
}
//////////// END code for GSM/GPRS registration ////////////

//////////// code for EEPROM read/write /////////////
/*
 * We store the current state and the history data that cannot be sent in the EEPROM
 * and tries to send them when the network is available.
 * 
 * EEPROM definition
 * GPS Data Block - 24 bytes per block
 *           ----------------------
 * 4  bytes  Lat Information in float      
 * 4  bytes  Lon Information in float
 * 14 bytes  UTC stamp of this record
 * 1 byte    Fix Level
 * 1 byte    End of Record
 * --------------------------------
 * ///////////////////////////////////////////
 *            --------------------
 * Base Addr. 0x0     | curr mode
 *            0x1-    | GPS blocks
 *            ......
 */

void write_GPS_data_to_EEPROM(float lat, float lon, char* utc, char fix, int offset) {
  EEPROM.put(offset, lat);
  offset += sizeof(float);
  EEPROM.put(offset, lon);
  offset += sizeof(float);
  int i;
  for (i = 0; i < 8; i++) {
    EEPROM.write(offset + i, '0');
  }
  for (i = 8; i < 14; i++) {
    EEPROM.write(offset + i, UTC[i - 8]);
  }
  EEPROM.write(offset + 14, fix);
  EEPROM.write(offset + 15, '\0');
}

void read_GPS_data_to_global_var(int offset) {
  EEPROM.get(offset, lat);
  offset += sizeof(float);
  EEPROM.get(offset, lon);
  offset += sizeof(float);
  for (int i = 8; i < 14; i++) {
    UTC[i - 8] = EEPROM.read(offset + i);
  }
  fix = EEPROM.read(offset + 14);
}
/////////// END code for EEPROM read/write ////////////

void setup() {
  // put your setup code here, to run once:
  pinMode(6, OUTPUT);
  digitalWrite(6, HIGH);
  delay(1000);
  digitalWrite(6, 0);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (wd_count > 0) {
    digitalWrite(6, 1);
    delay(1000);
    digitalWrite(6, 0);
  }
    enter_power_saving_mode();
}
