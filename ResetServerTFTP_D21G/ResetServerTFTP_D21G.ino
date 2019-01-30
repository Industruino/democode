// Reset Web Server
//
// A simple sketch that creates a web servers for remote resetting the
// Industruino D21G board thourgh a GET request to a specific URL:
//
//    http://{ip}[:{port}]/{reset_path}/reset
//
// {reset_path} is used instead of a password.  This is not a proper/secure
// authentication method, just a way to differentiate between different boards
// on the same local network if you need to.
//
// Originally created 14 Sep 2012 by Stelios Tsampas
// Adapted for Industruino by Claudio Indellicati and Tom Tobback

//////////////////////////////////////////////////////////////////
#define VERSION "v1"    // to verify the new version is uploaded
//////////////////////////////////////////////////////////////////

#include <SPI.h>                                // for Ethernet
#include <Ethernet2.h>
#include <EthernetReset.h>
#include <Wire.h>                               // for RTC EEPROM MAC
#include <UC1701.h>
static UC1701 lcd;

// Networking parameters for the reset server
byte mac[6];                                    // read from RTC EEPROM
IPAddress locIp(192, 168, 1, 199);
IPAddress dnsIp(192, 168, 1, 1);
IPAddress gwIp(192, 168, 1, 1);
IPAddress netMask(255, 255, 255, 0);
int port = 8080;
char *reset_path = "password";

// Create the reset server
EthernetReset reset(port, reset_path);

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

void setup()
{
  Wire.begin();           // for MAC from RTC EEPROM

  pinMode(26, OUTPUT);
  digitalWrite(26, HIGH);  // LCD full backlight

  lcd.begin();
  lcd.setCursor(0, 0);
  lcd.print("EthernetReset DEMO");
  lcd.setCursor(50, 1);
  lcd.print(VERSION);

  SerialUSB.begin(115200);
  delay(1000);
  SerialUSB.println();
  SerialUSB.println("EthernetReset DEMO");
  SerialUSB.println("Get unique MAC from RTC EEPROM");
  readMACfromRTC();             // MAC stored in RTC EEPROM

  // Starting the reset server.
  // The 'begin()' method takes care of everything, from initializing the
  // Ethernet Module to starting the web server for resetting.
  // This is why you should always start it before any other server you might
  // need to run
  reset.begin(mac, locIp, dnsIp, gwIp, netMask);

  // Display some data
  lcd.setCursor(0, 3);
  lcd.print("MAC: ");
  for (int u = 0; u < 6; u++) {
    lcd.print(mac[u], HEX);
    if (u < 5) lcd.print(":");
  }
  lcd.setCursor(0, 4);
  lcd.print("IP:  ");
  lcd.print(Ethernet.localIP());

  lcd.setCursor(0, 6);
  lcd.print("Last reset (s):");
}

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

void loop()
{
  // After the reset server is running the only thing needed is this command at
  // the beginning of the 'loop()' function in your sketch and it will take care
  // of checking for any reset request
  reset.check();

  // Put the rest of your code here
  lcd.setCursor(100, 6);
  lcd.print(millis() / 1000);

}

/////////////////////////////////////////////////////////////////////////
// the RTC has a MAC address stored in EEPROM - 8 bytes 0xf0 to 0xf7 >> USE OUI and correct part of EI
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

