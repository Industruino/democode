/*
  Industruino 4-20mA.ker test sketch for DHT
  this sketch:
  > reads temperature and humidity from the DHT
  > maps humidity to 4-10mA
  > maps temperature to 12-20mA
  > sends 2 seconds of temp, 2 seconds of humidity
  > if the DHT returns "nan" (not a number) then output 11mA

  Connections on 4-20mA.ker to DHT:
  1     DATA with pull-up resistor if not included in DHT module
  2     NC
  3     NC
  4     NC
  5     NC
  6     NC
  7     GND
  8     VCC
*/

#include "DHT.h"

#define DHTPIN 1     // what digital pin we're connected to

// Uncomment whatever type you're using!
#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

DHT dht(DHTPIN, DHTTYPE);

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void setup() {

  pinMode(0, OUTPUT);    // built-in LED
  digitalWrite(0, HIGH); // switch LED on
  delay(100);
  digitalWrite(0, LOW); // switch LED off

  SerialUSB.begin(115200);
  delay(2000); // allow some time for the USB connection to establish after upload, OR
  //while (!SerialUSB) {}    // alternative to the above line, wait for SerialUSB to connect (Serial Monitor), but this blocks if no Serial Monitor connected
  SerialUSB.println("4-20mA.ker DHT test");

  DAC->CTRLB.bit.REFSEL = 0x00; //Set DAC to external VREF (2.5V)
  analogWriteResolution(12);
  digitalWrite(PIN_EXT_3V3_ENABLE, HIGH);

  dht.begin();

}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void loop() {

   int microamps;
   
   digitalWrite(0, HIGH);             // LED on
   float t = dht.readTemperature();
   SerialUSB.print(t);
   SerialUSB.print("*C \t");
   microamps = map(t, 0, 50, 12000, 20000);           // MAPPING
   if (isnan(t)) microamps = 11000;
   SerialUSB.print(microamps/1000.0);
   SerialUSB.println("mA");   
   analogWrite(PIN_DAC0, uAmap(microamps));
   delay(2000);

   digitalWrite(0, LOW);             // LED off
   float h = dht.readHumidity();
   SerialUSB.print(h);
   SerialUSB.print("% \t\t");
   microamps = map(h, 40, 100, 4000, 10000);           // MAPPING
   if (isnan(h)) microamps = 11000;
   SerialUSB.print(microamps/1000.0);
   SerialUSB.println("mA");   
   analogWrite(PIN_DAC0, uAmap(microamps));
   delay(2000);

}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// helper function to map milliamps to 0-4095 DAC range
int uAmap(int ua) {
  return map(ua, 3800, 20700, 0, 4095); // map microamps 3.8-20.7mA
}


