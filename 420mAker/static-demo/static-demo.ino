/*
  Industruino 4-20mA.ker test sketch
  this sketch:
  > generates a constant 4-20mA signal
    the DAC is 12-bit: 0 to 4095, mapped to 3.8-20.7mA
  > blinks the built-in LED every 2 seconds
  
*/

const int LEDpin = 0;                // built-in LED
const float mA_value = 14.5;         // value for static output

void setup() {
  DAC->CTRLB.bit.REFSEL = 0x00;  //Set DAC to external VREF (2.5V)
  analogWriteResolution(12);
  pinMode(LEDpin, OUTPUT);
  for (int i = 0; i < 10; i++) {
    digitalWrite(LEDpin, HIGH);
    delay(50);
    digitalWrite(LEDpin, LOW);
    delay(50);
  }
  
  analogWrite(PIN_DAC0, mAmap(mA_value));
  
}

void loop() {

  digitalWrite(LEDpin, HIGH);
  delay(2000);
  digitalWrite(LEDpin, LOW);
  delay(2000);

}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// helper function to map milliamps to 0-4095 DAC range
int mAmap(float ma) {
  return map(1000 * ma, 3800, 20700, 0, 4095); // map microamps 3.8-20.7mA
}

