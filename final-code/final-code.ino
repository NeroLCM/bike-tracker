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
#define SLEEP_TIMES 220
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
  Serial.println(command);
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
      send_at_command_and_select("AT+CREG?", "+CREG: 1,1", "+CREG: 1,5", 3000) == 0 &&
      send_at_command_and_select("AT+CREG?", "+CREG: 0,1", "+CREG: 0,5", 3000) == 0
    );
  if (
    send_at_command_and_select("AT+CREG?", "+CREG: 1,1", "+CREG: 1,5", 3000) != 0 ||
    send_at_command_and_select("AT+CREG?", "+CREG: 0,1", "+CREG: 0,5", 3000) != 0
  ) {
    return 0;
  }
  return 1;
}

void setup_GPRS_param() {
  send_at_command_and_validate("AT+SAPBR=3,1,\"Contype\",\"GPRS\"", "OK", 3000);
  send_at_command_and_validate("AT+SAPBR=3,1,\"APN\",\"phone\"", "OK", 3000);
  int i = 0;
  while (send_at_command_and_validate("AT+SAPBR=1,1", "OK", 20000) == 0 && i++ < 3) delay(1000);
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
 *            0x3FF   | numBlocks
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

/////////// code for HTTP requests ////////////
#define ERR_ACTIONSETTINGS 2
#define ERR_URLSETTINGS 3
#define ERR_REQUEST 4
#define ERR_READDATA 1000
/*
* Sends an http request, pulls the respond to buffer
* and returns the respond code
*/
int send_http_get_request(char *url, char *buf) {
  int res;
  Serial.println("AT+HTTPINIT");
  res = send_at_command_and_validate("AT+HTTPPARA=\"CID\",1", "OK", 4000);
  if (res != 1) return ERR_ACTIONSETTINGS;
  char cmd_str[200];
  memset(cmd_str, '\0', 200);
  sprintf(cmd_str, "AT+HTTPPARA=\"URL\",\"%s\"", url);
  res = send_at_command_and_validate(cmd_str, "OK", 4000);
  if (res != 1) return ERR_URLSETTINGS;
  res = send_at_command_and_validate("AT+HTTPACTION=0", "+HTTPACTION:0,", 10000);
  if (res != 1) return ERR_REQUEST;
  while (Serial.available() < 4);
  // reads the respond code from buffer, reuse cmd_str space
  char tmp;
  res = 0;
  while (Serial.available() == 0);
  tmp = Serial.read();
  while (tmp != ',') {
    res = res * 10 + (tmp - 0x30);
    while (Serial.available() == 0);
    tmp = Serial.read();
  }
  // 200 - HTTP OK, get the first 100 bytes of data
  int i;
  i = send_at_command_and_select("AT+HTTPREAD=0,100", "+HTTPREAD:", "ERROR", 5000);
  buf[0] = '\0';
  if (i == 0) {
    Serial.println("AT+HTTPTERM");
    return res;
  } else if (i == 2) {
    Serial.println("AT+HTTPTERM");
    return res + ERR_READDATA;
  }
  int data_size = 0;
  while (Serial.available() == 0);
  tmp = Serial.read();
  while (tmp != 0x0D) {
    data_size = data_size * 10 + (tmp - 0x30);
    while (Serial.available() == 0);
    tmp = Serial.read();
  }
  i = 0;
  while (i < data_size+1) {
    if (Serial.available() > 0) {
      buf[i] = Serial.read();
      i++;
    }
  }
  buf[i] = '\0';
  send_at_command_and_validate("AT+HTTPTERM", "OK", 3000);
  return res;
}

/////////// END code for HTTP requests ////////////

#define HOME "lcm.im/ee149/index.php?token=598234c470cd87148cd26090f83ac4e6"
#define TESTDATA "lcm.im/ee149/data.php?lat=37.773246&lon=-122.158217&utc=171001"

////////// State Machine Control /////////
enum ctrl_state {
  STARTUP = 0,
  SLEEP_STANDBY = 1,
  DEEP_SLEEP = 2,
  NORMAL = 3,
  TRACKING = 4,
  RESUME = 5
};

enum ctrl_state state;
const int GSM_MODE_PIN = 2;
const int GPS_MODE_PIN = 3;
const int SLEEP_CTRL_PIN = 8;
const int ERR_LED_INDICATOR = 10;
const int BAT_VOLTAGE_PIN = 5;

const int GSM_MODE = 0;
const int GPS_MODE = 1;
void switch_mode(int mode) {
  if (mode == GPS_MODE) {
    digitalWrite(GSM_MODE_PIN, HIGH);
    digitalWrite(GPS_MODE_PIN, LOW);
  } else {
    digitalWrite(GSM_MODE_PIN, LOW);
    digitalWrite(GPS_MODE_PIN, HIGH);
  }
}
/////////////////////////////////////////

////////////// Power Management /////////
int bat_level = 0; // battery percentage

void updateBatLevel() {
  int raw_data = analogRead(BAT_VOLTAGE_PIN);
  // 0.0049V per unit, 7.4v -> 0, 8.4v -> 100
  float volt = raw_data * 0.0049;
  bat_level = (int) ((volt - 7.4) * 100);
}

/////////////////////////////////////////

void setup() {
  pinMode(SLEEP_CTRL_PIN, OUTPUT);
  pinMode(ERR_LED_INDICATOR, OUTPUT);
  digitalWrite(SLEEP_CTRL_PIN, LOW);
  digitalWrite(ERR_LED_INDICATOR, LOW);
  state = STARTUP;
}

void loop() {
  updateBatLevel();
  if (state == STARTUP) {
    Serial.begin(115200);
    switch_mode(GSM_MODE);
    while (send_at_command_and_validate("AT", "OK", 1000) == 0);
    Serial.println("AT+CGPSPWR=1"); // turn on GPS
    Serial.println("AT+CGPSRST=0"); // cold start GPS
    wait_for_GSM_registration(60000);
    delay(10);
    setup_GPRS_param();
    state = NORMAL;
  } else if (state == RESUME) {
    switch_mode(GSM_MODE);
    digitalWrite(SLEEP_CTRL_PIN, LOW);
    delay(1000);
    Serial.println("AT+CSCLK=0");
    Serial.println("AT+CGPSRST=1"); // hot start GPS
    state = NORMAL;
  } else if (state == SLEEP_STANDBY) {
    if (bat_level < 30) {
      state = DEEP_SLEEP;
      return;
    }
    Serial.println("AT+CGPSPWR=0");
    enter_power_saving_mode();
    state = RESUME;
  } else if (state == DEEP_SLEEP) {
    Serial.println("AT+CGPSPWR=0");
    Serial.println("AT+CSCLK=1");
    digitalWrite(SLEEP_CTRL_PIN, HIGH);
    enter_power_saving_mode();
    state = RESUME;
  } else if (state == TRACKING) {
    switch_mode(GSM_MODE);
    Serial.println("AT+CGPSPWR=1"); // turn on GPS
    wait_for_GSM_registration(60000);
    delay(10);
    char url_str[180];
    char buf[105] = {0};
    sprintf(
      url_str, 
      "lcm.im/ee149/index.php?token=598234c470cd87148cd26090f83ac4e6&bat=%d",
      bat_level
    );
    send_http_get_request(url_str, buf);
    if (buf[0] != 'T') { // NORMAL MODE
      state = NORMAL;
      return;
    }
    unsigned long time_limit = millis() + 360000; // alloc 6 min for fix
    char ret = 0;
    while (millis() < time_limit) {
      if (
        send_at_command_and_select(
          "AT+CGPSSTATUS?",
          "+CGPSSTATUS: Location 2D Fix",
          "+CGPSSTATUS: Location 3D Fix",
          3000
        ) != 0
      ) {
        ret = 1;
        break;
      }
    }
    if (ret == 0) {
      return; // retry, but check if it should still be in tracking mode first
    }
    switch_mode(GPS_MODE);
    delay(100);
    updateAllGPSData();
    switch_mode(GSM_MODE);
    delay(100);
    memset(url_str, '\0', 180);
    memset(buf, '\0', 105);
    sprintf(url_str, "lcm.im/ee149/data.php?lat=%f&lon=%f&utc=%s", lat, lon, UTC);
    send_http_get_request(url_str, buf);
    delay(5000);
    return; // retry, but check if it should still be in tracking mode first
  } else if (state == NORMAL) {
    switch_mode(GSM_MODE);
    Serial.println("AT+CGPSPWR=1"); // turn on GPS
    wait_for_GSM_registration(60000);
    delay(10);
    char url_str[180];
    char buf[105] = {0};
    sprintf(
      url_str, 
      "lcm.im/ee149/index.php?token=598234c470cd87148cd26090f83ac4e6&bat=%d",
      bat_level
    );
    send_http_get_request(url_str, buf);
    if (buf[0] == 'T') { // TRACKING MODE
      state = TRACKING;
      return;
    }
    unsigned long time_limit = millis() + 180000; // alloc 3 min for fix
    char ret = 0;
    while (millis() < time_limit) {
      if (
        send_at_command_and_select(
          "AT+CGPSSTATUS?",
          "+CGPSSTATUS: Location 2D Fix",
          "+CGPSSTATUS: Location 3D Fix",
          3000
        ) != 0
      ) {
        ret = 1;
        break;
      }
    }
    if (ret == 0) { // Fix failed, go to sleep
      state = SLEEP_STANDBY;
    } else {
      // switch to GPS mode
      switch_mode(GPS_MODE);
      delay(100);
      updateAllGPSData();
      switch_mode(GSM_MODE);
      delay(100);
      memset(url_str, '\0', 180);
      memset(buf, '\0', 105);
      sprintf(url_str, "lcm.im/ee149/data.php?lat=%f&lon=%f&utc=%s", lat, lon, UTC);
      send_http_get_request(url_str, buf);
      if (buf[0] == 'O' and buf[1] == 'K') { // OK!
        state = SLEEP_STANDBY;
        return;
      } else {
        char num_pending_block = EEPROM.read(0x3FF);
        EEPROM.write(0x3FF, num_pending_block + 1);
        int offset = 1 + 24 * num_pending_block;
        write_GPS_data_to_EEPROM(lat, lon, UTC, fix, offset);
        state = SLEEP_STANDBY;
        return;
      }
    }
  } else {
    Serial.println("INVALID STATE");
    digitalWrite(ERR_LED_INDICATOR, HIGH);
  }
}
