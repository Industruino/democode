// Adafruit Watchdog Library Basic Usage Example
// Simple example of how to use the watchdog library.
// Author: Tony DiCola
// modified for Industruino D21G by Tom Tobback, Aug 2017

#include <UC1701.h>   // Industruino LCD
static UC1701 lcd;

#include <Adafruit_SleepyDog.h>  // https://github.com/adafruit/Adafruit_SleepyDog
                                 // also need to install https://github.com/adafruit/Adafruit_ASFcore

void setup() {
  pinMode(26, OUTPUT);     // Industruino backlight
  digitalWrite(26, HIGH);  // full backlight
  
  lcd.begin();
  lcd.setCursor(0,0);
  lcd.print("Watchdog Library Demo");
  delay(2000);
  
  // First a normal example of using the watchdog timer.
  // Enable the watchdog by calling Watchdog.enable() as below.  This will turn
  // on the watchdog timer with a ~4 second timeout before reseting the Arduino.
  // The estimated actual milliseconds before reset (in milliseconds) is returned.
  // Make sure to reset the watchdog before the countdown expires or the Arduino
  // will reset!
  int countdownMS = Watchdog.enable(4000);
  lcd.setCursor(0,2);
  lcd.print("watchdog (ms): ");
  lcd.print(countdownMS, DEC);

  // Now loop a few times and periodically reset the watchdog.
  for (int i = 1; i <= 10; ++i) {
    lcd.setCursor(0,4);
    lcd.print("Loop #"); lcd.println(i, DEC);
    delay(1000);
    // Reset the watchdog with every loop to make sure the sketch keeps running.
    // If you comment out this call watch what happens after about 4 iterations!
    Watchdog.reset();
  }

  // Disable the watchdog entirely by calling Watchdog.disable();
  Watchdog.disable();

  // Finally demonstrate the watchdog resetting by enabling it for a shorter
  // period of time and waiting a long time without a reset.  Notice you can pass
  // a _maximum_ countdown time (in milliseconds) to the enable call.  The library
  // will try to use that value as the countdown, but it might pick a smaller
  // value if the hardware doesn't support it.  The actual countdown value will
  // be returned so you can see what it is.
  countdownMS = Watchdog.enable(4000);
  lcd.setCursor(0,5);
  lcd.print("Long loop");
  delay(countdownMS+1000);

  // Execution will never get here because the watchdog resets the Arduino!
}

void loop() {
  // We'll never actually get to the loop because the watchdog will reset in
  // the setup function.
  lcd.setCursor(0,5);
  lcd.print("You shouldn't see this message.");
  delay(1000);
}
