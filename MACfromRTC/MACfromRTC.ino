/*
 * Industruino example sketch
 * The D21G topboard has a built-in RTC (MCP79402) with a EUI-64 number that can be used as MAC address
 * This sketch:
 * -reads 6-byte (48-bit) MAC address from RTC EEPROM over I2C
 * -starts Ethernet with MAC address and static IP address
 * October 2018
 */
 
#include <Wire.h>                               // for RTC
#include <SPI.h>                                // for Ethernet
#include <Ethernet2.h>
byte mac[6];                                    // read from RTC
IPAddress industruino_ip (192, 168, 1, 100);    // example

void setup() {
  Wire.begin();
  SerialUSB.begin(115200);
  while (!SerialUSB);          // sketch waits here for Serial Monitor
  
  SerialUSB.println();
  SerialUSB.println("Get unique MAC from RTC EEPROM"); 
  readMACfromRTC();             // mac stored in rtc eeprom
  Ethernet.begin(mac, industruino_ip);  
  SerialUSB.print("Ethernet started with IP: ");
  SerialUSB.println(Ethernet.localIP());
}

void loop() {
}

/////////////////////////////////////////////////////////////////////////
// the RTC has a MAC address stored in EEPROM - 8 bytes 0xf0 to 0xf7
void readMACfromRTC() {
  SerialUSB.println("READING MAC from RTC EEPROM");
  int mac_index = 0;
  for (int i = 0; i < 8; i++) {   // read 8 bytes of 64-bit MAC address, 3 bytes valid OUI, 5 bytes unique EI
    byte m = readByte(0x57, 0xf0 + i);
    SerialUSB.print(m, HEX);
    if (i < 7) SerialUSB.print(":");
    if (i != 3 && i != 4) {       // for 6-bytes MAC, skip first 2 bytes of EI
      mac[mac_index] = m;
      mac_index++;
    }      
  }
  SerialUSB.println();
  SerialUSB.print("Extracted 6-byte MAC address: ");
  for (int u = 0; u < 6; u++) {
    SerialUSB.print(mac[u], HEX);
    if (u < 5) SerialUSB.print(":");
  }
  SerialUSB.println();
}

/////////////////////////////////////////////////////////////////////////
// the RTC has a MAC address stored in EEPROM
uint8_t readByte(uint8_t i2cAddr, uint8_t dataAddr) {
  Wire.beginTransmission(i2cAddr);
  Wire.write(dataAddr);
  Wire.endTransmission(false); // don't send stop
  Wire.requestFrom(i2cAddr, 1);
  return Wire.read();
}
