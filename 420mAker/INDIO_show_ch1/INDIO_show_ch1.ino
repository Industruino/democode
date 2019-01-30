/*
 * Industruino demo sketch for use with INDIO and 4-20mA.ker
 * this sketch:
 * > reads 4-20mA signal on analog input CH1
 * > displays the value on the LCD and Serial Monitor
 */

#include <Indio.h>
#include <Wire.h>

#include <UC1701.h>
static UC1701 lcd;

float ch1;     // to hold value of analog input channel 1

void setup()
{
  SerialUSB.begin(115200);

  pinMode(26, OUTPUT);     // LCD backlight
  digitalWrite(26, HIGH);  // full backlight

  lcd.begin();
  lcd.setCursor(0, 1);
  lcd.print("ANALOG INPUT");

  Indio.setADCResolution(12); // Set the ADC resolution. Choices are 12bit@240SPS, 14bit@60SPS, 16bit@15SPS and 18bit@3.75SPS.
  Indio.analogReadMode(1, mA);
}

void loop()
{

  ch1 = Indio.analogRead(1); //Read Analog-In CH1 (output depending on selected mode)
  SerialUSB.print("CH1 (mA): "); //Print "CH" for human readability
  SerialUSB.println(ch1, 3); //Print data

  lcd.setCursor(0, 3);
  lcd.print("CH1 (mA): ");
  lcd.print(ch1, 3);

  lcd.setCursor(0, 5);
  lcd.print("time (sec): ");
  lcd.print(millis() / 1000);
  
}

