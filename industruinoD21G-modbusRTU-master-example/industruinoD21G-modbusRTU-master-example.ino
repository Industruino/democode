/*
  INDUSTRUINO IND.I/O topboard d21g
  RS485 connection: Modbus RTU protocol
  Industruino = Modbus Master
  sensor = Modbus Slave
  this example reads holding registers (function code 3), from a certain START_OF_SLAVE_INDEX, with total of registers TOTAL_NO_OF_REGISTERS

  more info on the SimpleModbusMaster library: https://drive.google.com/folderview?id=0B0B286tJkafVYnBhNGo4N3poQ2c&usp=drive_web&tid=0B0B286tJkafVSENVcU1RQVBfSzg#list

  Tom Tobback for Industruino
  March 2021
*/
#include <SimpleModbusMaster.h>

//////////////////// Port information ///////////////////
#define baud 9600                          // SENSOR SPEC
#define timeout 1000
#define polling 200 // the scan rate
#define retry_count 10
#define slave_id 2                         // SENSOR SPEC
#define serial_type SERIAL_8N1             // SENSOR SPEC: SERIAL_8N2 or SERIAL_8E1 or ..  (SERIAL_8N1 is common, e.g. default for Arduino)
// used to toggle the receive/transmit pin on the driver
#define TxEnablePin 9                      // INDUSTRUINO RS485

// check your sensor spec for the register map, if numbers are 40000+ then subtract the offset 40001
#define START_OF_SLAVE_INDEX 0            // SENSOR SPEC
// The total amount of available memory on the master to store data
#define TOTAL_NO_OF_REGISTERS 16          // SENSOR SPEC

// This is the easiest way to create new packets
// Add as many as you want. TOTAL_NO_OF_PACKETS is automatically updated.
enum
{
  PACKET1,                                          // this example shows only 1 type of operation: read the sensor (FC3)
  TOTAL_NO_OF_PACKETS // leave this last entry
};

// Create an array of Packets to be configured
Packet packets[TOTAL_NO_OF_PACKETS];

// Masters register array
unsigned int regs[TOTAL_NO_OF_REGISTERS];
unsigned long ts = 0;

void setup()
{
  // see SENSOR SPEC on which function to use to retrieve/set data registers
  // READ_COIL_STATUS = FC1                        
  // READ_INPUT_STATUS = FC2                        
  // READ_HOLDING_REGISTERS = FC3     // most common                   
  // READ_INPUT_REGISTERS = FC4       // most common                 
  // FORCE_SINGLE_COIL = FC5                   
  // PRESET_SINGLE_REGISTER = FC6                        
  // FORCE_MULTIPLE_COILS = FC15                        
  // PRESET_MULTIPLE_REGISTERS = FC16
  // Initialize each packet: packet, slave-id, function, start of slave index, number of regs, start of master index
  modbus_construct(&packets[PACKET1], slave_id, READ_HOLDING_REGISTERS, START_OF_SLAVE_INDEX, TOTAL_NO_OF_REGISTERS, 0);   

  // Initialize the Modbus Finite State Machine
  modbus_configure(&Serial, baud, serial_type, timeout, polling, retry_count, TxEnablePin, packets, TOTAL_NO_OF_PACKETS, regs);

  SerialUSB.begin(115200);
  pinMode(26, OUTPUT);  // Industruino LCD backlight
  digitalWrite(26, HIGH);
}

void loop()
{
  modbus_update();                          // send 1 simple Master request to Slave, as defined above
  packets[0].connection = true;           // make sure connection never drops after x retries

  if (millis() - ts > 1000) {
    for (int i = 0; i < TOTAL_NO_OF_REGISTERS; i++) {
      SerialUSB.print(regs[i]);
      SerialUSB.print(" ");
    }
    SerialUSB.println();
    ts = millis();
  }

  delay(10);

}
