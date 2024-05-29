/*
  General functions for Industruino

  Libraries needed:
  > Indio: https://github.com/Industruino/Indio
  > UC1701: https://github.com/Industruino/UC1701
  > WDTzero: https://github.com/javos65/WDTZero instead of adafruit sleepydog, limited to 16s, resets for wifi no-ssl
  > PubSubClient: https://github.com/knolleary/pubsubclient
  > Ethernet: see eth tab
  > WiFiNINA: see wifi tab
*/

// extract FILENAME from full path in __FILE__
#define FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

// Industruino MAC
#include <Wire.h>
byte mac[6];       // 6 bytes extracted from 8 bytes unique number in EEPROM
String indio_mac;  // unique identifier based on 6 byte MAC address

// Industruino LCD
#include <UC1701.h>
static UC1701 lcd;
#define LCD_BACKLIGHT 26
#define ENTER_PIN 24
#define DOWN_PIN 23
#define UP_PIN 25

// Industruino I/O
#include <Indio.h>
volatile boolean digital_channel_change_flag = 0;

// Industruino SD card on ETH, GSM, WIFI modules
#include <SPI.h>
#include <SD.h>
const int SD_CS = 4;
const int FRAM_COUNTER_ADDRESS_START = 0;  // 4x 4-byte unsigned long

// Industruino FRAM non-volatile memory on ETH, GSM, WIFI modules
const byte FRAM_CMD_WREN = 0x06;   //0000 0110 Set Write Enable Latch
const byte FRAM_CMD_WRDI = 0x04;   //0000 0100 Write Disable
const byte FRAM_CMD_RDSR = 0x05;   //0000 0101 Read Status Register
const byte FRAM_CMD_WRSR = 0x01;   //0000 0001 Write Status Register
const byte FRAM_CMD_READ = 0x03;   //0000 0011 Read Memory Data
const byte FRAM_CMD_WRITE = 0x02;  //0000 0010 Write Memory Data
const int FRAM_CS1 = 6;            //chip select 1

/////////////////////// WATCHDOG TIMER ////////////////////////////
#include <WDTZero.h>
WDTZero myWDT;  // Define WDT

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void WDTshutdown() {
  SerialUSB.println();
  SerialUSB.println("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
  SerialUSB.println("+++  WDT shutdown! 2 minutes stuck somewhere.. +++++++++++++++++++++++");
  SerialUSB.println("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
  SerialUSB.println();
  delay(100);
}

//////////////////////////// MAC ADDRESS /////////////////////////////////////////////
// the RTC has a MAC address stored in EEPROM
uint8_t readByte(uint8_t i2cAddr, uint8_t dataAddr) {
  Wire.beginTransmission(i2cAddr);
  Wire.write(dataAddr);
  Wire.endTransmission(false);  // don't send stop
  Wire.requestFrom(i2cAddr, 1);
  return Wire.read();
}

//////////////////////////// MAC ADDRESS ////////////////////////////////////////////
// the RTC has a MAC address stored in EEPROM - 8 bytes 0xf0 to 0xf7
void readMACfromRTC() {
  Wire.begin();  // for MAC in RTC eeprom
  SerialUSB.print("[MAC] reading 8-byte MAC from RTC EEPROM: ");
  int mac_index = 0;
  for (int i = 0; i < 8; i++) {  // read 8 bytes of 64-bit MAC address, 3 bytes valid OUI, 5 bytes unique EI
    byte m = readByte(0x57, 0xf0 + i);
    // Miklos reports that sometimes this returns 0xFF which is not correct, so let's read again in that case
    if (m == 0xFF) {  // check for glitch
      delay(100);
      m = readByte(0x57, 0xf0 + i);
    }
    SerialUSB.print(m, HEX);
    if (i < 7) SerialUSB.print(":");
    if (i != 3 && i != 4) {  // for 6-bytes MAC, skip first 2 bytes of EI
      mac[mac_index] = m;
      mac_index++;
    }
    delay(1);  // just to avoid glitches
  }
  SerialUSB.println();
  SerialUSB.print("[MAC] extracted 6-byte MAC address: ");
  for (int u = 0; u < 6; u++) {
    SerialUSB.print(mac[u], HEX);
    if (u < 5) SerialUSB.print(":");
  }
  SerialUSB.println();
}

///////////////////////////////////////////////////////////////////
/// FRAM functions need to be included before setup()
///////////////////////////////////////////////////////////////////
int FRAMWrite(int addr, byte *buf, int count = 1) {
  if (addr > 0x7ff) return -1;
  byte addrMSB = (addr >> 8) & 0xff;
  byte addrLSB = addr & 0xff;
  digitalWrite(FRAM_CS1, LOW);
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  SPI.transfer(FRAM_CMD_WREN);  //write enable
  digitalWrite(FRAM_CS1, HIGH);
  digitalWrite(FRAM_CS1, LOW);
  SPI.transfer(FRAM_CMD_WRITE);  //write command
  SPI.transfer(addrMSB);
  SPI.transfer(addrLSB);
  for (int i = 0; i < count; i++) SPI.transfer(buf[i]);
  digitalWrite(FRAM_CS1, HIGH);
  return 0;
}

int FRAMRead(int addr, byte *buf, int count = 1) {
  if (addr > 0x7ff) return -1;
  byte addrMSB = (addr >> 8) & 0xff;
  byte addrLSB = addr & 0xff;
  digitalWrite(FRAM_CS1, LOW);
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  SPI.transfer(FRAM_CMD_READ);
  SPI.transfer(addrMSB);
  SPI.transfer(addrLSB);
  for (int i = 0; i < count; i++) buf[i] = SPI.transfer(0x00);
  digitalWrite(FRAM_CS1, HIGH);
  return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void digital_channel_change_ISR() {
  digital_channel_change_flag = 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void configIO() {

  // DIGITAL CH1-4: inputs = "binary_sensor"
  for (int i = 1; i <= 4; i++) {
    Indio.digitalMode(i, INPUT);
    SerialUSB.print("[INDIO] digital channel ");
    SerialUSB.print(i);
    SerialUSB.println(" set to INPUT");
  }

  // DIGITAL CH5-8: outputs = "switch"
  for (int i = 5; i <= 8; i++) {
    Indio.digitalMode(i, OUTPUT);
    //Indio.digitalWrite(i, LOW);    // get initial value via retained MQTT message
    SerialUSB.print("[INDIO] digital channel ");
    SerialUSB.print(i);
    SerialUSB.println(" set to OUTPUT");
  }

  // attach an interrupt for digital channel change
  //attachInterrupt(8, digital_channel_change_ISR, FALLING);       // D8 attached to the interrupt of the expander

  // ANALOG INPUT CH1-4: "sensor"
  // V10      0-10V
  // V10_p    0-100% for 0-10V
  // V10_raw  0-4095 for 0-10V
  // mA       0-20mA
  // mA_p     0-100% for 4-20mA
  // mA_raw   0-4095 for 0-20mA
  Indio.setADCResolution(12);  // Set the ADC resolution. Choices are 12bit@240SPS, 14bit@60SPS, 16bit@15SPS and 18bit@3.75SPS.
  for (int i = 1; i <= 4; i++) {
    Indio.analogReadMode(i, V10_p);  // Set Analog-In to % 10V mode (0-10V -> 0-100%).
    SerialUSB.print("[INDIO] analog input channel ");
    SerialUSB.print(i);
    SerialUSB.println(" set to V10_p (0-10V -> 0-100%)");
  }

  // ANALOG OUTPUT CH1-2: "number"
  // V10      0-10V
  // V10_p    0-100% for 0-10V
  // V10_raw  0-4095 for 0-10V
  // mA       0-20mA
  // mA_p     0-100% for 4-20mA
  // mA_raw   0-4095 for 0-20mA
  for (int i = 1; i <= 2; i++) {
    Indio.analogWriteMode(i, V10_p);  // Set Analog-Out to % 10V mode (0-10V -> 0-100%).
    //Indio.analogWrite(i, 0, false);  // get initial value via retained MQTT message
    SerialUSB.print("[INDIO] analog output channel ");
    SerialUSB.print(i);
    SerialUSB.println(" set to V10_p (0-10V -> 0-100%)");
  }
}

//////////////////////////////////////////////////////////////////////////////////////////

uint32_t readFRAMulong(byte addr) {

  union ul_4bytes {  // define a union structure to convert ulong into 4 bytes and back
    unsigned long ul;
    byte b[4];
  };

  union ul_4bytes union_ulong_read;

  FRAMRead(addr, (byte *)union_ulong_read.b, 4);

  return union_ulong_read.ul;
}

//////////////////////////////////////////////////////////////////////////////////////////

void writeFRAMulong(byte addr, uint32_t val_ulong) {

  union ul_4bytes {  // define a union structure to convert ulong into 4 bytes and back
    unsigned long ul;
    byte b[4];
  };

  union ul_4bytes union_ulong_write;  // create a variable of this union type
  union_ulong_write.ul = val_ulong;   // asign the value

  FRAMWrite(addr, (byte *)union_ulong_write.b, 4);

  /*
  // this is to check if the write is successful, or we assume it works
  union ul_4bytes union_ulong_read;  
  FRAMRead(addr, (byte*) union_ulong_read.b, 4);

  if (union_ulong_read.ul != val_ulong) {
    SerialUSB.println(">>> WARNING: EEPROM write did not work");
  } else {
    //SerialUSB.println("[EEPROM write successful]");
  }
*/
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void initSD_FRAM_WDT_MAC() {

  // membrane button pins have pull-up resistor
  pinMode(UP_PIN, INPUT);     // button
  pinMode(ENTER_PIN, INPUT);  // button
  pinMode(DOWN_PIN, INPUT);   // button

  // FRAM, SD share SPI bus so we make sure all chip select pins are HIGH at the start
  pinMode(FRAM_CS1, OUTPUT);
  pinMode(SD_CS, OUTPUT);
  digitalWrite(FRAM_CS1, HIGH);
  digitalWrite(SD_CS, HIGH);
  SPI.begin();  // to use FRAM
  // enable WDT
  myWDT.attachShutdown(WDTshutdown);
  myWDT.setup(WDT_SOFTCYCLE2M);  // initialize WDT-softcounter refesh cycle on 32sec interval WDT_SOFTCYCLE32S
  SerialUSB.println("[WDT] watchdog timer started, max 2 minutes");
  myWDT.clear();
  readMACfromRTC();
  // create unique identifier from 6 byte MAC
  indio_mac = "";
  for (int i = 2; i < 6; i++) {
    if (mac[i] < 0x10) indio_mac += "0";
    String digit = "";
    digit = String(mac[i], HEX);
    digit.toUpperCase();
    indio_mac += digit;
  }
  SerialUSB.print("[MAC] using 4-byte unique indio_mac: ");
  SerialUSB.println(indio_mac);

  // get pulse counters from FRAM
  SerialUSB.println("[FRAM] retrieving digital input pulse counters:");
  for (int i = 1; i <= 4; i++) {
    dig_in_pulse_counter[i] = readFRAMulong(FRAM_COUNTER_ADDRESS_START + (i - 1) * 4);
    SerialUSB.print("[FRAM] pulse counter digital channel ");
    SerialUSB.print(i);
    SerialUSB.print(": ");
    SerialUSB.println(dig_in_pulse_counter[i]);
  }
}
