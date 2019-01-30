/*
  Industruino 4-20mA.ker test sketch for RC522 RFID
  this sketch:
  > reads a Mifare Classic card, and recognises 2 card IDs
  > if no reader is found at startup, it outputs 10mA and keeps trying to initiate a reader
  > if reader is working but no card is found, it outputs 12mA
  > if card A is recognised, it outputs 20mA for 1 second
  > if card B is recognised, it outputs 17mA for 1 second
  > if another card is found, it outputs 14mA for 1 second

  Based on a MFRC522 library example; for further details and other examples see: https://github.com/miguelbalboa/rfid
  The library files need to be modified for use with SAMD/SAML cores: use SerialUSB instead of Serial
  Go to the src folder and find/replace "Serial." with "SerialUSB." in the .cpp files

  Connections on 4-20mA.ker to RC522:
  1     MISO
  2     SS/SDA
  3     MOSI
  4     SCK
  5     RST
  6     NC
  7     GND
  8     3.3V
*/


#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 2
#define RST_PIN 5

MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
MFRC522::MIFARE_Key key;

// 2 preset cards to recognise
byte cardA[] = { 0x71, 0xB1, 0xD0, 0x2E };
byte cardB[] = { 0x45, 0x27, 0x0C, 0x19 };

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
  SerialUSB.println("4-20mA.ker RFID test");

  DAC->CTRLB.bit.REFSEL = 0x00; //Set DAC to external VREF (2.5V)
  analogWriteResolution(12);
  digitalWrite(PIN_EXT_3V3_ENABLE, HIGH);

  SPI.begin(); // Init SPI bus, needed for RC522
  rfid.PCD_Init(); // Init MFRC522

  while (!rfid.PCD_PerformSelfTest()) {     // as long as reader is not working, output 10mA and reset reader
    SerialUSB.println("Cannot find RC522 reader..");
    analogWrite(PIN_DAC0, mAmap(10));   // output 10mA
    rfid.PCD_Init(); // Init MFRC522
  }
  rfid.PCD_Init(); // Init MFRC522 - necessary after SelfTest

  SerialUSB.println("RC522 reader connected correctly");
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  SerialUSB.println(F("This code looks for MIFARE Classic cards"));

  SerialUSB.print("preset card A = ");
  printHex(cardA, 4);
  SerialUSB.println();
  SerialUSB.print("preset card B = ");
  printHex(cardB, 4);
  SerialUSB.println();
  SerialUSB.println();

  for (int i = 0; i < 10; i++) {
    digitalWrite(0, HIGH); // switch LED on
    delay(50);
    digitalWrite(0, LOW); // switch LED off
    delay(50);
  }

}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void loop() {

  // default level for DAC
  analogWrite(PIN_DAC0, mAmap(12));   // output 12mA
  digitalWrite(0, LOW); // switch LED off

  // Look for new cards
  if ( ! rfid.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial())
    return;

  //SerialUSB.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  //SerialUSB.println(rfid.PICC_GetTypeName(piccType));

  // Check is the PICC of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
      piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
      piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    SerialUSB.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  if (rfid.uid.uidByte[0] == cardA[0] ||
      rfid.uid.uidByte[1] == cardA[1] ||
      rfid.uid.uidByte[2] == cardA[2] ||
      rfid.uid.uidByte[3] == cardA[3] ) {
    SerialUSB.println("card A detected");
    analogWrite(PIN_DAC0, mAmap(20));  // output 20mA
  } else {
    if (rfid.uid.uidByte[0] == cardB[0] ||
        rfid.uid.uidByte[1] == cardB[1] ||
        rfid.uid.uidByte[2] == cardB[2] ||
        rfid.uid.uidByte[3] == cardB[3] ) {
      SerialUSB.println("card B detected");
      analogWrite(PIN_DAC0, mAmap(17));  // output 17mA
    } else {
      SerialUSB.println("unknown card detected");
      analogWrite(PIN_DAC0, mAmap(14));  // output 14mA
    }
  }
  
  digitalWrite(0, HIGH); // switch LED on
  delay(1000);

  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// helper routine to dump a byte array as hex values to SerialUSB.
void printHex(byte * buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    SerialUSB.print(buffer[i] < 0x10 ? " 0" : " ");
    SerialUSB.print(buffer[i], HEX);
  }
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// helper function to map milliamps to 0-4095 DAC range
int mAmap(float ma) {
  return map(1000 * ma, 3800, 20700, 0, 4095); // map microamps 3.8-20.7mA
}

