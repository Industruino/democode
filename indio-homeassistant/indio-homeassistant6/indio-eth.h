/*
  ETHERNET functions for Industruino
  using the standard Arduino Ethernet library

  Library needed:
  > Ethernet: Arduino version at https://github.com/arduino-libraries/Ethernet
      with one important modification in the standard library at /home/tom/.arduino15/libraries/Ethernet/src/utility/w5100.h
      change SPI speed line 48(?) from 8 to 4MHz
      #define SPI_ETHERNET_SETTINGS SPISettings(4000000, MSBFIRST, SPI_MODE0)     // Industruino 4MHz
    note: compile with verbose output to find the location of the Ethernet library your IDE is using
*/

#include <Ethernet.h>  // use new general Arduino version
EthernetClient eth_client;

/////////////// ETH CONFIG PARAMETERS ///////////////////////////////////////////////////////////////
#define USE_DHCP 1
IPAddress industruino_ip(192, 168, 8, 100);  // without DHCP, and also used as fallback in case DHCP fails
/////////////////////////////////////////////////////////////////////////////////////////////////////

void initEthernet() {

  SerialUSB.println("[ETH] run initEthernet()..");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("[ETH] init");

  // show Industruino MAC from EEPROM
  SerialUSB.print("[ETH] MAC: ");
  SerialUSB.print(mac[5], HEX);
  SerialUSB.print(":");
  SerialUSB.print(mac[4], HEX);
  SerialUSB.print(":");
  SerialUSB.print(mac[3], HEX);
  SerialUSB.print(":");
  SerialUSB.print(mac[2], HEX);
  SerialUSB.print(":");
  SerialUSB.print(mac[1], HEX);
  SerialUSB.print(":");
  SerialUSB.println(mac[0], HEX);
  lcd.setCursor(0, 1);
  lcd.print("mac ");
  lcd.print(mac[5], HEX);
  lcd.print(":");
  lcd.print(mac[4], HEX);
  lcd.print(":");
  lcd.print(mac[3], HEX);
  lcd.print(":");
  lcd.print(mac[2], HEX);
  lcd.print(":");
  lcd.print(mac[1], HEX);
  lcd.print(":");
  lcd.print(mac[0], HEX);

  // new Ethernet library can detect cable status
  auto link = Ethernet.linkStatus();
  SerialUSB.print("[ETH] link status: ");
  switch (link) {
    case Unknown:
      SerialUSB.println("Unknown, is the ETHERNET module connected?");
      lcd.setCursor(0, 3);
      lcd.print("check ETH module?");
      while (1)
        ;
      break;
    case LinkON:
      SerialUSB.println("ON");
      break;
    case LinkOFF:
      SerialUSB.println("OFF, is the ETHERNET cable plugged in?");
      lcd.setCursor(0, 3);
      lcd.print("check ETH cable?");
      while (1)
        ;
      break;
  }


  // start Ethernet
  lcd.setCursor(0, 2);
  if (USE_DHCP) {
    SerialUSB.print("[ETH] requesting IP address from DHCP... ");
    lcd.print("requesting IP (DHCP)");
    lcd.setCursor(0, 3);
    if (!Ethernet.begin(mac)) {
      SerialUSB.println("could not get IP address over DHCP, use static IP");
      lcd.print("DHCP failed, using static");
      Ethernet.begin(mac, industruino_ip);
    }
    SerialUSB.println("OK");
    lcd.print("OK");
  } else {  // static IP
    SerialUSB.println("[ETH] using static IP (no DHCP)");
    lcd.print("using static IP");
    Ethernet.begin(mac, industruino_ip);
  }
  SerialUSB.print("[ETH] IP address: ");
  SerialUSB.println(Ethernet.localIP());
  lcd.setCursor(0, 4);
  lcd.print("IP ");
  lcd.print(Ethernet.localIP());

  delay(1500);  // for displaying
}
