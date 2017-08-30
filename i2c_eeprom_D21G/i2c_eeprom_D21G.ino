/*
 * Industruino D21G EEPROM demo
 * Tom Tobback Aug 2017
 * 
 * The EEPROM has 4 I2C addresses:
 * 0x50, 0x51, 0x52, 0x53
 * each I2C address has 255 addresses of 1 byte = 255 bytes 
 * in total 4x 255 bytes are available = 1kByte 
 * 
 * Use the I2C_eeprom library from https://github.com/RobTillaart/Arduino/tree/master/libraries/I2C_EEPROM
 * 
 * below sketch reads/writes examples of: bytes, long, float
 */
 
#include <Wire.h>
#include <I2C_eeprom.h>

#define EEPROM_SIZE 255

I2C_eeprom eeprom50(0x50, EEPROM_SIZE);
I2C_eeprom eeprom51(0x51, EEPROM_SIZE);
I2C_eeprom eeprom52(0x52, EEPROM_SIZE);
I2C_eeprom eeprom53(0x53, EEPROM_SIZE);


void setup() {
  SerialUSB.begin(9600);
  while (!SerialUSB);    // wait for Serial Monitor

  eeprom50.begin();
  eeprom51.begin();
  eeprom52.begin();
  eeprom53.begin();

  SerialUSB.println("INDUSTRUINO D21G I2C EEPROM DEMO");
  SerialUSB.println("================================");
  SerialUSB.println();
  
  ///////////////////////////////////////////////////////////////////////////////////////
  SerialUSB.println("Writing a fixed byte value to all addresses at I2C address 0x50..");
  byte val = 77;
  for (int i=0;i<=EEPROM_SIZE;i++) {
    eeprom50.writeByte(i, val);
    delay(1);   // NEEDED between successive writings
  }

  delay(1000);
  
  SerialUSB.println("Reading from all addresses at I2C address 0x50..");
  byte read_val;
  SerialUSB.print("addr:\t");
  for (int i=0;i<=EEPROM_SIZE;i++) {
    SerialUSB.print(i);
    SerialUSB.print("\t");
  }
  SerialUSB.println();
  SerialUSB.print("val:\t");
  for (int i=0;i<=EEPROM_SIZE;i++) {
    read_val = eeprom50.readByte(i);
    SerialUSB.print(read_val);
    SerialUSB.print("\t");
//    delay(1);   // seems not essential
  }
  SerialUSB.println();
  SerialUSB.println();

  /////////////////////////////////////////////////////////////////////////////////////////////////
  SerialUSB.print("Writing a LONG type number to address 10-13 (4 bytes) at I2C address 0x51:");

  long long_number = -987654321;
  SerialUSB.println(long_number);

  union l_4bytes {    // define a union structure to convert long into 4 bytes and back
    long l;
    byte b[4];
  };

  union l_4bytes union_long_write;   // create a variable of this union type
  union_long_write.l = long_number;  // asign the value

  eeprom51.writeByte(10, union_long_write.b[0]);
  delay(1);
  eeprom51.writeByte(11, union_long_write.b[1]);
  delay(1);
  eeprom51.writeByte(12, union_long_write.b[2]);
  delay(1);
  eeprom51.writeByte(13, union_long_write.b[3]);
  
  delay(1000);

  SerialUSB.print("Reading LONG type from I2C address 0x51: ");

  union l_4bytes union_long_read;
  union_long_read.b[0] = eeprom51.readByte(10);
  union_long_read.b[1] = eeprom51.readByte(11);
  union_long_read.b[2] = eeprom51.readByte(12);
  union_long_read.b[3] = eeprom51.readByte(13);

  SerialUSB.println(union_long_read.l);
  SerialUSB.println();


  /////////////////////////////////////////////////////////////////////////////////////////////////
  SerialUSB.print("Writing a FLOAT type number to address 252-255 (4 bytes) at I2C address 0x53:");

  float float_number = -123.456;
  SerialUSB.println(float_number, 3);

  union f_4bytes {      // define a union structure to convert float into 4 bytes and back
    float f;
    byte b[4];
  };

  union f_4bytes union_float_write;     // create a variable of this union type
  union_float_write.f = float_number;   // assign the value

  eeprom53.writeByte(252, union_float_write.b[0]);
  delay(1);
  eeprom53.writeByte(253, union_float_write.b[1]);
  delay(1);
  eeprom53.writeByte(254, union_float_write.b[2]);
  delay(1);
  eeprom53.writeByte(255, union_float_write.b[3]);
  
  delay(1000);

  SerialUSB.print("Reading FLOAT type from I2C address 0x53: ");

  union f_4bytes union_float_read;
  union_float_read.b[0] = eeprom53.readByte(252);
  union_float_read.b[1] = eeprom53.readByte(253);
  union_float_read.b[2] = eeprom53.readByte(254);
  union_float_read.b[3] = eeprom53.readByte(255);

  SerialUSB.println(union_float_read.f, 3);
  SerialUSB.println();
  SerialUSB.println("END");

}

void loop() {
  // nothing here
}
