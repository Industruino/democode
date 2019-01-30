/*
  Industruino 4-20mA.ker test sketch for DS18B20
  this sketch:
  > reads temperature 
  > maps temperature 0-100*C to 4-20mA

  Connections on 4-20mA.ker to DS18B20:
  1     NC
  2     NC
  3     NC
  4     NC
  5     NC
  6     DATA  with pull-up resistor 4K7 to VCC
  7     GND
  8     VCC
*/

#include <OneWire.h>
OneWire  ds(6);            // on pin 6 (a pull-up resistor of 4.7K or 5K resistor is necessary, 10K does not work)

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
  SerialUSB.println("4-20mA.ker DS18B20 test");

  DAC->CTRLB.bit.REFSEL = 0x00; //Set DAC to external VREF (2.5V)
  analogWriteResolution(12);
  digitalWrite(PIN_EXT_3V3_ENABLE, HIGH);

}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void loop() {

   int microamps;

   digitalWrite(0, HIGH);        // LED on
   float t = readDS();
   //SerialUSB.print(t);
   //SerialUSB.print("*C \t");
   microamps = map(t * 100, 0 * 100, 100 * 100, 4000, 20000);           // MAPPING only does integer values, use factor 100 for precision
   microamps = constrain(microamps, 4000, 20000);    // mapping may be out of range
   SerialUSB.print(microamps/1000.0, 3);
   SerialUSB.println("mA");   
   analogWrite(PIN_DAC0, uAmap(microamps));
   digitalWrite(0, LOW);         // LED off
   delay(1000);

}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// helper function to map milliamps to 0-4095 DAC range
// better use microcamps because the mapping function only deals with integer values
int uAmap(int ua) {
  return map(ua, 3800, 20700, 0, 4095); // map microamps 3.8-20.7mA
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float readDS(void) {
  byte i;
  byte present = 0;
  byte data[12];
  byte addr[8];
  int HighByte, LowByte, TReading, SignBit;
  float Tc;

  ds.reset_search();
  if ( !ds.search(addr)) {
      SerialUSB.print("No more addresses.\n");
      ds.reset_search();
      return 0;
  }

  if ( OneWire::crc8( addr, 7) != addr[7]) {
      SerialUSB.print("CRC is not valid!\n");
      return 0;
  }

  if ( addr[0] == 0x10) {
      SerialUSB.print("DS18S20: ");
  }
  else if ( addr[0] == 0x28) {
      SerialUSB.print("DS18B20: ");
  }
  else {
      SerialUSB.print("Device family is not recognized: 0x");
      SerialUSB.println(addr[0],HEX);
      return 0;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44,1);         // start conversion, with parasite power on at the end

  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.

  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
  }
  
  LowByte = data[0];
  HighByte = data[1];
  TReading = (HighByte << 8) + LowByte;
  SignBit = TReading & 0x8000;  // test most sig bit
  if (SignBit) // negative
  {
    TReading = (TReading ^ 0xffff) + 1; // 2's comp
  }
  Tc = TReading * 0.0625;    // multiply by (100 * 0.0625) or 6.25

  if (SignBit) // If its negative
  {
     Tc = -Tc;
  }
  SerialUSB.print(Tc, 2);  // easy way
  SerialUSB.print("*C\t");
  return Tc;
}

