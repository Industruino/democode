/*
  Test sketch for INDUSTRUINO WIFI expansion module

  BEFORE YOU UPLOAD THIS SKETCH

  1) ESP32 needs to run the NINA firmware (adafruit version)
  download the .bin file from https://github.com/adafruit/nina-fw/releases/latest
  do not connect the module to the Industruino yet, instead connect the module to your laptop directly via the microUSB socket
  upload this .bin file to your ESP32-IDC module with esptool
  use esptool to upload:
     install esptool the easy way with pip: https://github.com/espressif/esptool
     or if you have the ESP32 core installed in your Arduino IDE, look for esptool.py in ~/.arduino15/packages/esp32/tools/esptool_py/2.6.1 or similar
  from the command line: esptool.py --port /dev/ttyUSB0 --baud 115200 write_flash 0 ~/Downloads/NINA_W102-1.7.4.bin

  2) get the WiFiNINA library (adafruit version) from https://github.com/adafruit/WiFiNINA
  it is necessary to update 1 line in the library: sketchbook/libraries/WiFiNINA-master/src/utility/spi_drv.cpp
  we need to change the SPI speed down from 8MHz to 4MHz, so the line should read:
       WIFININA_SPIWIFI->beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
  the library is documented at https://www.arduino.cc/en/Reference/WiFiNINA

  3) this sketch uses the standard SD library included with the Arduino IDE

  4) we don't use the FRAM library, the basic read/write functions are defined below

  FUNCTION OF THIS SKETCH

  SETUP
  > shows the NINA firmware version of the WIFI module
  > scans for wireless networks
  > connects to a specific network
  LOOP
  > init SD and checks the content of the SD card
  > tests FRAM and set flag addr250 to 7
  > connect to a server: www.httpbin.org over SSL (or not if specified)

  note: TCP connection takes about 300ms on port 80 and 4-10sec for SSL on port 443

  Tom Tobback, May 2023
*/

/////////////// CONFIG PARAMETERS ///////////////////////////////////////////////////////////////
char ssid[] = "XXXX";         // your network SSID (name)
char pass[] = "XXXX";         // your network password (use for WPA, or use as key for WEP)
#define USE_SSL 1             // 0 no SSL (port 80), 1 for SSL (port 443) -- this selects the wificlient
/////////////////////////////////////////////////////////////////////////////////////////////////

// Industruino LCD
#include <UC1701.h>
static UC1701 lcd;
#define LCD_BACKLIGHT 26
#define ENTER_PIN 24

// Industruino WIFI module
#include <SPI.h>
#include <WiFiNINA.h>
#include <utility/wifi_drv.h>  // for module RED LED
#define ESP32_RGB_RED 26
#define SPIWIFI_SS 10
#define SPIWIFI_ACK 7
#define ESP32_RESETN 5
#define ESP32_GPIO0 -1  // was 6 but D6 needed for FRAM CS
#define SPIWIFI SPI
byte mac[6];  // the MAC address of the wifi module

// example URL for GET request
char server[] = "www.httpbin.org";  // example test server, available both SLL and not
char resource[] = "/ip";            // see https://httpbin.org/ for api options

// initiate wifi client depending on SSL or not
#if USE_SSL == 1
WiFiSSLClient client;  // for client that always uses SSL
int port = 443;
#else
WiFiClient client;  // for default client, can still use connect() or connectSSL()
int port = 80;
#endif

// Industruino WIFI module SD card
#include <SD.h>
const int SD_CS = 4;
const char *datafile_name = "/test.txt";  // file will be created on SD card

// Industruino WIFI module FRAM non-volatile memory
const byte FRAM_CMD_WREN = 0x06;   //0000 0110 Set Write Enable Latch
const byte FRAM_CMD_WRDI = 0x04;   //0000 0100 Write Disable
const byte FRAM_CMD_RDSR = 0x05;   //0000 0101 Read Status Register
const byte FRAM_CMD_WRSR = 0x01;   //0000 0001 Write Status Register
const byte FRAM_CMD_READ = 0x03;   //0000 0011 Read Memory Data
const byte FRAM_CMD_WRITE = 0x02;  //0000 0010 Write Memory Data
const int FRAM_CS1 = 6;            //chip select 1

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

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

void setup() {

  // backlight on Industruino LCD
  pinMode(LCD_BACKLIGHT, OUTPUT);
  digitalWrite(LCD_BACKLIGHT, HIGH);
  // enter pin on membrane panel
  pinMode(ENTER_PIN, INPUT);

  // WIFI, FRAM, SD all share SPI bus so we make sure all 3 chip select pins are HIGH at the start
  pinMode(FRAM_CS1, OUTPUT);
  pinMode(SD_CS, OUTPUT);
  pinMode(SPIWIFI_SS, OUTPUT);
  digitalWrite(FRAM_CS1, HIGH);
  digitalWrite(SD_CS, HIGH);
  digitalWrite(SPIWIFI_SS, HIGH);

  SerialUSB.begin(115200);
  delay(2000);
  SerialUSB.println();
  SerialUSB.println("==============================");
  SerialUSB.println("Industruino WIFI module tester");
  SerialUSB.println("==============================");

  lcd.begin();
  lcd.clear();
  lcd.print("WIFI module test");

  initWifi();

  // wait for enter button press on the Industruino LCD panel
  lcd.setCursor(0, 7);
  lcd.print("press ENTER");
  SerialUSB.println("press ENTER to continue");
  while (digitalRead(ENTER_PIN));
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

void loop() {

  lcd.clear();
  lcd.print("WIFI module test");
  SerialUSB.println();

  testSD();

  testFRAM();

  getRequest();

  SerialUSB.print("[NTP] network time: ");
  printTime(WiFi.getTime());

  // show uptime while waiting for 8 seconds, then repeat test
  lcd.setCursor(0, 7);
  lcd.print("uptime:");
  unsigned long this_ts = millis();
  while (millis() - this_ts < 8000) {
    lcd.setCursor(80, 7);
    lcd.print(millis() / 1000);  // show uptime seconds
    delay(100);
  }
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

void listNetworks() {  // for wifi scan
  // scan for nearby networks:
  SerialUSB.println("==Scan networks==");
  int numSsid = WiFi.scanNetworks();
  if (numSsid == -1) {
    SerialUSB.println("couldn't find any wifi connection, stop here");
    lcd.setCursor(0, 3);
    lcd.print("no networks found, STOP");
    while (true)
      ;  // stay here forever
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

void printEncryptionType(int thisType) {  // used by wifi scan
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

void printTime(unsigned long unix_time) {  // to convert unix timestamp to readable time
  SerialUSB.print("Unix time = ");
  SerialUSB.print(unix_time);
  // print the hour, minute and second:
  SerialUSB.print(" = UTC time ");               // UTC is the time at Greenwich Meridian (GMT)
  SerialUSB.print((unix_time % 86400L) / 3600);  // print the hour (86400 equals secs per day)
  SerialUSB.print(':');
  if (((unix_time % 3600) / 60) < 10) {
    // In the first 10 minutes of each hour, we'll want a leading '0'
    SerialUSB.print('0');
  }
  SerialUSB.print((unix_time % 3600) / 60);  // print the minute (3600 equals secs per minute)
  SerialUSB.print(':');
  if ((unix_time % 60) < 10) {
    // In the first 10 seconds of each minute, we'll want a leading '0'
    SerialUSB.print('0');
  }
  SerialUSB.println(unix_time % 60);  // print the second
}

//////////////////////////////////////////////////////////////////////////////////////////

void printDirectory(File dir, int numTabs) {  // used for SD card
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      SerialUSB.print('\t');
    }
    SerialUSB.print(entry.name());
    if (entry.isDirectory()) {
      SerialUSB.println(" / ");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      SerialUSB.print("\t\t");
      SerialUSB.println(entry.size(), DEC);
    }
    entry.close();
  }
}

//////////////////////////////////////////////////////////////////////////////////////////

void initWifi() {

  lcd.setCursor(0, 1);
  lcd.print("initWifi");

  // configure WIFI pins
  WiFi.setPins(SPIWIFI_SS, SPIWIFI_ACK, ESP32_RESETN, ESP32_GPIO0, &SPIWIFI);  // specific to Industruino WIFI module
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
    lcd.print("no wifi module found");
    while (true)
      ;  // stay here forever
  }

  // initiate the RED LED on WIFI module and blink it
  WiFiDrv::pinMode(ESP32_RGB_RED, OUTPUT);
  // blink RED LED
  for (int i = 0; i < 5; i++) {
    WiFiDrv::digitalWrite(ESP32_RGB_RED, HIGH);  // on
    delay(100);
    WiFiDrv::digitalWrite(ESP32_RGB_RED, LOW);  // off
    delay(100);
  }

  // check MAC address of the WIFI module
  WiFi.macAddress(mac);
  SerialUSB.print("[WIFI] MAC: ");
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
  lcd.print("MAC ");
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
  lcd.print("SSID: ");
  lcd.print(ssid);
  SerialUSB.print("[WIFI] connecting to SSID: ");
  SerialUSB.println(ssid);
  lcd.setCursor(0, 4);
  lcd.print("connecting..");
  // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
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
  WiFiDrv::digitalWrite(ESP32_RGB_RED, LOW);  // RED LED off

  IPAddress ip = WiFi.localIP();
  SerialUSB.print("[WIFI] IP Address: ");
  SerialUSB.println(ip);
  lcd.setCursor(0, 5);
  lcd.print("IP:");
  lcd.print(ip);

  long rssi = WiFi.RSSI();
  SerialUSB.print("[WIFI] signal strength (RSSI): ");
  SerialUSB.print(rssi);
  SerialUSB.println("dBm");
  lcd.setCursor(0, 6);
  lcd.print("RSSI: ");
  lcd.print(rssi);
  lcd.print("dBm");
}

//////////////////////////////////////////////////////////////////////////////////////////

void testSD() {
  // look for SD card
  SerialUSB.print("[SD] Initializing SD card... ");
  lcd.setCursor(0, 1);
  lcd.print("test SD:");

  lcd.setCursor(80, 1);
  if (!SD.begin(SD_CS)) {
    SerialUSB.println("no SD card found");
    lcd.print("n/a");
  } else {  // if SD card found, list files
    SerialUSB.println("OK");
    File root = SD.open("/");
    SerialUSB.println("[SD] contents of SD card:");
    printDirectory(root, 0);
    root.close();
    SerialUSB.print("[SD] save current UNIX timestamp to ");
    SerialUSB.println(datafile_name);
    File dataFile = SD.open(datafile_name, FILE_WRITE);
    unsigned long current_unix_timestamp = WiFi.getTime();
    dataFile.println(current_unix_timestamp);
    dataFile.close();
    // open file again for reading
    dataFile = SD.open(datafile_name, FILE_READ);
    SerialUSB.println("[SD] content of " + String(datafile_name));
    SerialUSB.println("<<< start of file >>>");
    while (dataFile.available()) {
      String this_line = dataFile.readStringUntil('\n');
      SerialUSB.println(this_line);
    }
    SerialUSB.println("<<< end of file >>>");
    dataFile.close();
    lcd.print("OK");
  }
}

//////////////////////////////////////////////////////////////////////////////////////////

void testFRAM() {
  lcd.setCursor(0, 2);
  lcd.print("test FRAM");
  // configure wifi flag in FRAM, address 250 set to 7
  byte read_flag[1];
  read_flag[0] = 7;
  FRAMWrite(250, read_flag, 1);
  SerialUSB.println("[FRAM] setting FRAM address 250 to 7 as wifi module flag");
  read_flag[0] = 0;
  FRAMRead(250, read_flag, 1);  // address 250
  lcd.setCursor(80, 2);
  if (read_flag[0] == 7) {  // 7 is code for wifi
    SerialUSB.println("[FRAM] flag set and read successfully");
    lcd.print("OK");
  } else {
    SerialUSB.println("[FRAM] flag set failed!!");
    lcd.print("FAIL!!");
  }
}

//////////////////////////////////////////////////////////////////////////////////////////

void getRequest() {
  // DO HTTP GET REQUEST
  lcd.setCursor(0, 3);
  if (USE_SSL) lcd.print("test HTTPS");
  else lcd.print("test HTTP");
  SerialUSB.print("[TCP] connecting to site: ");
  SerialUSB.print(server);
  SerialUSB.print(" on port: ");
  SerialUSB.println(port);

  lcd.setCursor(0, 4);
  unsigned long start_ts = millis();
  if (client.connect(server, port)) {  // for HTTPS connection over SSL
    //if (client.connect(server, 80)) {       // for HTTP connection
    //if (client.connectSSL(server, 443)) {      // if we would use the standard WiFiClient we could still connect SSL
    WiFiDrv::digitalWrite(ESP32_RGB_RED, LOW);  // RED LED off
    SerialUSB.print("[TCP] connected in ");
    SerialUSB.print((millis() - start_ts));
    SerialUSB.println("ms");
    lcd.print("time: ");
    lcd.print((millis() - start_ts));
    lcd.print("ms");
    // make a HTTP request:
    client.print("GET ");
    client.print(resource);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(server);
    client.println("Connection: close");
    client.println();
    // wait for the reply, with timeout
    unsigned long tm_ts = millis();
    while (client.available() == 0 && millis() - tm_ts < 2000)
      ;  // wait for reply up to 2s
    // the reply will be a json object like {"origin": "112.119.155.238"}
    lcd.setCursor(80, 3);
    if (client.find("\"origin\":")) {
      String origin = client.readStringUntil('\n');
      origin.trim();
      SerialUSB.print("[TCP] received reply: ");
      SerialUSB.println(origin);
      lcd.print("OK");
    } else {
      SerialUSB.println("[TCP] received no valid reply");
      lcd.print("FAIL!!");
    }
    while (client.available()) client.read();  // flush the client
    client.stop();
    SerialUSB.println("[TCP] disconnect");
  } else {
    WiFiDrv::digitalWrite(ESP32_RGB_RED, HIGH);  // RED LED on
    SerialUSB.println("[TCP] connection failed");
    lcd.print("connection FAIL!!");
    int status = WiFi.status();
    SerialUSB.print("[WIFI] WiFi.status(): ");
    SerialUSB.println(status);  // showed 5 and 6: WL_CONNECT_FAILED and WL_CONNECTION_LOST?
  }
}
