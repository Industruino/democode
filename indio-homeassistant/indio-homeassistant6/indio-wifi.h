/*
  WIFI functions for Industruino

  Library needed:
  > WiFiNINA library (adafruit version) from https://github.com/adafruit/WiFiNINA
  it is necessary to update 1 line in the library: sketchbook/libraries/WiFiNINA-master/src/utility/spi_drv.cpp
  we need to change the SPI speed down from 8MHz to 4MHz, so the line should read:
       WIFININA_SPIWIFI->beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
*/

/////////////// WIFI CONFIG PARAMETERS ///////////////////////////////////////////////////////////////
const char ssid[] = "tobback";        // your network SSID (name)
const char pass[] = "halloween11";    // your network password (use for WPA, or use as key for WEP)
/////////////////////////////////////////////////////////////////////////////////////////////////

// Industruino WIFI module
#include <WiFiNINA.h>
#include <utility/wifi_drv.h>   // for module RED LED
#define ESP32_RGB_RED    26
#define SPIWIFI_SS       10
#define SPIWIFI_ACK       7
#define ESP32_RESETN      5
#define ESP32_GPIO0       -1  // was 6 but D6 needed for FRAM CS
#define SPIWIFI          SPI
byte wifi_mac[6];                     // the MAC address of the wifi module

// initiate wifi client depending on SSL or not
#if USE_SSL == 1
WiFiSSLClient wifi_client;     // for client that always uses SSL
#else
WiFiClient wifi_client;        // for default client, can still use connect() or connectSSL()
#endif

///////////////////////////////////////////////////////////////////////

void printEncryptionType(int thisType) {    // used by wifi scan
  // read the encryption type and print out the name:
  switch (thisType) {
    case ENC_TYPE_WEP:
      SerialUSB.println("WEP");
      break;
    case ENC_TYPE_TKIP:
      SerialUSB.println("WPA");
      break;
    case ENC_TYPE_CCMP:
      SerialUSB.println("WPA2");
      break;
    case ENC_TYPE_NONE:
      SerialUSB.println("None");
      break;
    case ENC_TYPE_AUTO:
      SerialUSB.println("Auto");
      break;
    case ENC_TYPE_UNKNOWN:
    default:
      SerialUSB.println("Unknown");
      break;
  }
}

///////////////////////////////////////////////////////////////////////

void listNetworks() {         // for wifi scan
  // scan for nearby networks:
  SerialUSB.println("==Scan networks==");
  int numSsid = WiFi.scanNetworks();
  if (numSsid == -1) {
    SerialUSB.println("couldn't find any wifi connection, stop here");
    lcd.setCursor(0, 3);
    lcd.print("no networks found, STOP");
    while (true);  // stay here forever
  }

  // print the list of networks seen:
  SerialUSB.print("number of available networks:");
  SerialUSB.println(numSsid);

  // print the network number and name for each network found:
  for (int thisNet = 0; thisNet < numSsid; thisNet++) {
    SerialUSB.print(thisNet);
    SerialUSB.print(") ");
    SerialUSB.print(WiFi.SSID(thisNet));
    SerialUSB.print("\tSignal: ");
    SerialUSB.print(WiFi.RSSI(thisNet));
    SerialUSB.print(" dBm");
    SerialUSB.print("\tEncryption: ");
    printEncryptionType(WiFi.encryptionType(thisNet));
  }
  SerialUSB.println("==End of scan==");
}

///////////////////////////////////////////////////////////////////////

void initWifi() {

  SerialUSB.println("[WIFI] run initWifi()..");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("[WIFI] init");

  // hard reset of ESP32, not needed at startup, but maybe later when connection fails
  digitalWrite(ESP32_RESETN, LOW);
  delay(100);
  digitalWrite(ESP32_RESETN, HIGH);
  delay(1000);

  // configure WIFI pins
  WiFi.setPins(SPIWIFI_SS, SPIWIFI_ACK, ESP32_RESETN, ESP32_GPIO0, &SPIWIFI);   // specific to Industruino WIFI module
  // find wifi module, with timeout 5sec
  SerialUSB.print("[WIFI] connecting to wifi module..");
  unsigned long start_ts = millis();
  while (WiFi.status() == WL_NO_MODULE && millis() - start_ts < 5000) {
    SerialUSB.print(".");
    delay(500);
  }

  // check WIFI module status
  if (WiFi.status() != WL_NO_MODULE) {
    SerialUSB.println("found");
  } else {
    SerialUSB.println("NOT FOUND, stop here");
    lcd.setCursor(0, 2);
    lcd.print("no wifi module, STOP");
    while (true); // stay here forever
  }

  // check MAC address of the WIFI module
  WiFi.macAddress(wifi_mac);
  SerialUSB.print("[WIFI] MAC: ");
  SerialUSB.print(wifi_mac[5], HEX);
  SerialUSB.print(":");
  SerialUSB.print(wifi_mac[4], HEX);
  SerialUSB.print(":");
  SerialUSB.print(wifi_mac[3], HEX);
  SerialUSB.print(":");
  SerialUSB.print(wifi_mac[2], HEX);
  SerialUSB.print(":");
  SerialUSB.print(wifi_mac[1], HEX);
  SerialUSB.print(":");
  SerialUSB.println(wifi_mac[0], HEX);
  lcd.setCursor(0, 1);
  lcd.print("mac ");
  lcd.print(wifi_mac[5], HEX);
  lcd.print(":");
  lcd.print(wifi_mac[4], HEX);
  lcd.print(":");
  lcd.print(wifi_mac[3], HEX);
  lcd.print(":");
  lcd.print(wifi_mac[2], HEX);
  lcd.print(":");
  lcd.print(wifi_mac[1], HEX);
  lcd.print(":");
  lcd.print(wifi_mac[0], HEX);

  // check WIFI module firmware
  String fv = WiFi.firmwareVersion();
  SerialUSB.print("[WIFI] module firmware: ");
  SerialUSB.println(fv);
  lcd.setCursor(0, 2);
  lcd.print("firmware: ");
  lcd.print(fv);

  // scan and list available wifi access points (on SerialUSB only)
  //listNetworks();

  // attempt to connect to wifi network as defined above:
  lcd.setCursor(0, 3);
  lcd.print("connecting to SSID: ");
  lcd.setCursor(0, 4);
  lcd.print(ssid);
  SerialUSB.print("[WIFI] connecting to SSID: ");
  SerialUSB.println(ssid);
  lcd.setCursor(0, 5);
  int status = WL_IDLE_STATUS;
  bool led_status = false;
  do {
    WiFiDrv::digitalWrite(ESP32_RGB_RED, led_status);  // blink RED LED
    status = WiFi.begin(ssid, pass);
    lcd.print(".");
    SerialUSB.print(".");
    delay(500);
    led_status = !led_status;
  } while (status != WL_CONNECTED);
  SerialUSB.println();

  SerialUSB.print("[WIFI] connected to wifi network: ");
  SerialUSB.println(WiFi.SSID());  // just to double check it is the correct SSID
  lcd.print("OK");
  WiFiDrv::digitalWrite(ESP32_RGB_RED, LOW);   // RED LED off

  IPAddress ip = WiFi.localIP();
  SerialUSB.print("[WIFI] IP Address: ");
  SerialUSB.println(ip);
  lcd.setCursor(0, 6);
  lcd.print("IP:");
  lcd.print(ip);

  long rssi = WiFi.RSSI();
  SerialUSB.print("[WIFI] signal strength (RSSI): ");
  SerialUSB.print(rssi);
  SerialUSB.println("dBm");
  lcd.setCursor(0, 7);
  lcd.print("RSSI: ");
  lcd.print(rssi);
  lcd.print("dBm");

/*
  // use the wifi library's time function
  delay(1000);  // maybe necessary to let ntp do its work
  SerialUSB.println("[WIFI] retrieving network time..");
  byte tries = 0;
  while (tries < 5) {
    current_unix_timestamp = WiFi.getTime(); // this does not always work from the first time, so we tries a few times
    if (current_unix_timestamp) break;
    tries++;
    delay(1000);
  }
  if (current_unix_timestamp) printTime(current_unix_timestamp);
  else SerialUSB.println("[WIFI] WiFi.getTime() did not return a valid current timestamp yet..");
*/
  delay(1000);   // for displaying
}
