/*
 * PINOUT
 * HTS221 INT = 0
 * LPS25 INT = 1
 * XIAN SWITCH = A4
 * GPS SWITCH = A5
 * SD CHIP SELECT PIN = 4
 * RDY = 6
 * RDY = 7
 * 
 */

// IO Definitions
const int ATLAS_SWITCH = A3;
const int XIAN_SWITCH = A4;
const int GPS_SWITCH = A5;
const int RS485_SWITCH = A6;
const int SD_CS_PIN = 4;
const int MAX_3485_EN = 3;
const int EC_SWITCH = 0;
const int GPS_MOSFET = 2;
const int RESET_SWITCH = 6;

void declare_pins()
{
  pinMode(XIAN_SWITCH, OUTPUT);
  pinMode(GPS_SWITCH, OUTPUT);
  pinMode(MAX_3485_EN, OUTPUT);
  pinMode(RS485_SWITCH, OUTPUT);
  pinMode(EC_SWITCH, OUTPUT);
  pinMode(GPS_MOSFET, OUTPUT);
  pinMode(RESET_SWITCH,OUTPUT);
  digitalWrite(XIAN_SWITCH, LOW);
  digitalWrite(GPS_SWITCH, LOW);
  digitalWrite(MAX_3485_EN, LOW);
  digitalWrite(RS485_SWITCH, LOW);
  digitalWrite(RESET_SWITCH,LOW);
  digitalWrite(EC_SWITCH, LOW);
  digitalWrite(GPS_MOSFET,LOW); 
}
 
