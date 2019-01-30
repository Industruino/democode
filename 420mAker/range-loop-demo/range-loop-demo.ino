/*
  Industruino 4-20mA.ker test sketch
  this sketch generates a 4-20mA signal with linear up and down loop
  the DAC is 12-bit: 0 to 4095
  actual range is 3.8-20.7mA
*/

const int LEDpin = 0;           // built-in LED

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
}

void loop() {
  // from min to max
  for (int fadeValue = 0 ; fadeValue <= 4095; fadeValue++) {
    analogWrite(PIN_DAC0, fadeValue);
    delay(5);
  }

  digitalWrite(LEDpin, HIGH);
  delay(2000);
  digitalWrite(LEDpin, LOW);

  // from max to min
  for (int fadeValue = 4095 ; fadeValue >= 0; fadeValue--) {
    analogWrite(PIN_DAC0, fadeValue);
    delay(5);
  }

  digitalWrite(LEDpin, HIGH);
  delay(2000);
  digitalWrite(LEDpin, LOW);

}


