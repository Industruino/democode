//this sketch writes the string "This is a test" to the FRAM memory, reads it back and post the result to the serial terminal.

#include <SPI.h>

const byte CMD_WREN = 0x06; //0000 0110 Set Write Enable Latch
const byte CMD_WRDI = 0x04; //0000 0100 Write Disable
const byte CMD_RDSR = 0x05; //0000 0101 Read Status Register
const byte CMD_WRSR = 0x01; //0000 0001 Write Status Register
const byte CMD_READ = 0x03; //0000 0011 Read Memory Data
const byte CMD_WRITE = 0x02; //0000 0010 Write Memory Data

const int FRAM_CS1 = 6; //chip select 1

/**
 * Write to FRAM (assuming 2 FM25C160 are used)
 * addr: starting address
 * buf: pointer to data
 * count: data length. 
 *        If this parameter is omitted, it is defaulted to one byte.
 * returns: 0 operation is successful
 *          1 address out of range
 */
int FRAMWrite(int addr, byte *buf, int count=1)
{


  if (addr > 0x7ff) return -1;

  byte addrMSB = (addr >> 8) & 0xff;
  byte addrLSB = addr & 0xff;

  digitalWrite(FRAM_CS1, LOW);   
  SPI.transfer(CMD_WREN);  //write enable 
  digitalWrite(FRAM_CS1, HIGH);

  digitalWrite(FRAM_CS1, LOW);
  SPI.transfer(CMD_WRITE); //write command
  SPI.transfer(addrMSB);
  SPI.transfer(addrLSB);

  for (int i = 0;i < count;i++) SPI.transfer(buf[i]);

  digitalWrite(FRAM_CS1, HIGH);

  return 0;
}

/**
 * Read from FRAM (assuming 2 FM25C160 are used)
 * addr: starting address
 * buf: pointer to data
 * count: data length. 
 *        If this parameter is omitted, it is defaulted to one byte.
 * returns: 0 operation is successful
 *          1 address out of range
 */
int FRAMRead(int addr, byte *buf, int count=1)
{

  if (addr > 0x7ff) return -1;

  byte addrMSB = (addr >> 8) & 0xff;
  byte addrLSB = addr & 0xff;

  digitalWrite(FRAM_CS1, LOW);

  SPI.transfer(CMD_READ);
  SPI.transfer(addrMSB);
  SPI.transfer(addrLSB);

  for (int i=0; i < count; i++) buf[i] = SPI.transfer(0x00);

  digitalWrite(FRAM_CS1, HIGH);

  return 0;
}

void setup()
{
  pinMode(10, OUTPUT); 
  pinMode(6, OUTPUT); 
  pinMode(4, OUTPUT); 

  digitalWrite(10, HIGH);
  digitalWrite(6, HIGH);
  digitalWrite(4, HIGH);
  
  SerialUSB.begin(9600);  
  while (!SerialUSB) {
    ; // wait for SerialUSBUSB port to connect. Needed for native USB port only
  }
  pinMode(FRAM_CS1, OUTPUT);
  digitalWrite(FRAM_CS1, HIGH);


  //Setting up the SPI bus
  SPI.begin();
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));

  //Test
  char buf[]="This is a test";

  FRAMWrite(1, (byte*) buf, 14);  
  FRAMRead(1, (byte*) buf, 14);

  for (int i = 0; i < 14; i++) SerialUSB.print(buf[i]);
}

void loop()
{
}

