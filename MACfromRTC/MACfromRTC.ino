/*
 * Industruino example sketch
 * The D21G topboard has a built-in RTC with a EUI-48/EUI-64 number that can be used as MAC address
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
  for (int i = 0; i < 6; i++) {   // use 6 bytes for MAC address, skipping first 2 of 8: 0xf2 to 0xf7
    mac[i] = readByte(0x57, 0xf2 + i);
      SerialUSB.print(mac[i], HEX);
      if (i < 5) SerialUSB.print(":");
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
