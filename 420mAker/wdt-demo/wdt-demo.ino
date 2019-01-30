/*
  Industruino 4-20mA.ker test sketch for WDT watchdog timer
  this sketch:
  > blinks the built-in LED 10 times at startup
  > sets the WDT period to 2 seconds
  > Serial Monitor shows the time elapsed since WDT enable 
  > without the CLEAR line the WDT causes a reset after 2 seconds, with the CLEAR line the WDT never causes a reset

  NOTE: when the WDT causes a reset, the SerialUSB port is disconnected
  
  Connections on 4-20mA.ker to MPU6050:
  1     NC
  2     NC
  3     NC
  4     NC
  5     NC
  6     NC
  7     NC
  8     NC
*/


unsigned long timestamp;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void setup() {

  pinMode(0, OUTPUT);    // built-in LED
  for (int i = 0; i < 10; i++) {
    digitalWrite(0, HIGH); // switch LED on
    delay(100);
    digitalWrite(0, LOW); // switch LED off
    delay(100);
  }

  SerialUSB.begin(115200);
  delay(2000); // allow some time for the USB connection to establish after upload, OR
  //while (!SerialUSB) {}    // alternative to the above line, wait for SerialUSB to connect (Serial Monitor), but this blocks if no Serial Monitor connected
  SerialUSB.println("4-20mA.ker WDT test");

  DAC->CTRLB.bit.REFSEL = 0x00; //Set DAC to external VREF (2.5V)
  analogWriteResolution(12);
  digitalWrite(PIN_EXT_3V3_ENABLE, HIGH);

  //  delay(5000);  // to allow upload before WDT is enabled

  WDT->CTRLA.reg = 0;                       // init
  WDT->CONFIG.reg = WDT_CONFIG_PER_CYC2048; // set timeout period based on 1.024kHz clock, from 8ms to 16sec, CYC = 1ms
  // options: WDT_CONFIG_PER_CYC8 WDT_CONFIG_PER_CYC16 WDT_CONFIG_PER_CYC32 WDT_CONFIG_PER_CYC64 WDT_CONFIG_PER_CYC128
  // WDT_CONFIG_PER_CYC256 WDT_CONFIG_PER_CYC512 WDT_CONFIG_PER_CYC1024 WDT_CONFIG_PER_CYC2048 WDT_CONFIG_PER_CYC4096
  // WDT_CONFIG_PER_CYC8192 WDT_CONFIG_PER_CYC16384

  WDT->CTRLA.reg = WDT_CTRLA_ENABLE;        // enable

  timestamp = millis();

}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void loop() {

  SerialUSB.print("time since WDT enable: ");
  SerialUSB.println(millis() - timestamp);
  delay(1);
  
//  WDT->CLEAR.reg = WDT_CLEAR_CLEAR_KEY;   // clear the WDT: without this line, the WDT causes a reset after 2 seconds

}

