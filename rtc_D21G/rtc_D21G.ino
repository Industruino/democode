/*
  This software is licensed under a Creative Commons Attribution-ShareAlike 3.0 Unported License
  Attribution-ShareAlike
  CC BY-SA

  serial clock, example for MCP7940 rtc library
  version 1.0 / 2014/01/27
  http://smi.aii.pub.ro/arduino.html

  added LCD display for Industruino D21G
  Tom Tobback Aug 2017
*/

#include <Wire.h>
#include <MCP7940.h>

#include <UC1701.h>
static UC1701 lcd;

int rtc[7];

void setup()
{
  pinMode(26, OUTPUT);
  digitalWrite(26, HIGH);  // full backlight

  lcd.begin();
  lcd.setCursor(0, 0);
  lcd.print("RTC Demo");

  Wire.begin();
  SerialUSB.begin(9600);

  RTCind.get(rtc, true);
  // the following 7 lines are used to set the time,
  // only upload this code once uncommented to set the time,
  // afterwards comment these lines out and upload again.
  /*
    RTCind.set(MCP7940_SEC, 0);
    RTCind.set(MCP7940_MIN, 12);
    RTCind.set(MCP7940_HR, 15);
    RTCind.set(MCP7940_DOW, 3);
    RTCind.set(MCP7940_DATE, 30);
    RTCind.set(MCP7940_MTH, 8);
    RTCind.set(MCP7940_YR, 17);
  */
  // true for MCP7940N vith back-up battery
  // false for MCP7940M or MCP7940N vith VBAT connected to ground
  RTCind.start(true);
}

void loop()
{
  RTCind.get(rtc, true);

  lcd.setCursor(0, 3);

  switch (rtc[3]) {
    case 1:
      SerialUSB.print("MON ");
      lcd.print("Mon ");
      break;
    case 2:
      SerialUSB.print("TUE ");
      lcd.print("Tue ");
      break;
    case 3:
      SerialUSB.print("WED ");
      lcd.print("Wed ");
      break;
    case 4:
      SerialUSB.print("THU ");
      lcd.print("Thu ");
      break;
    case 5:
      SerialUSB.print("FRI ");
      lcd.print("Fri ");
      break;
    case 6:
      SerialUSB.print("SAT ");
      lcd.print("Sat ");
      break;
    case 7:
      SerialUSB.print("SUN ");
      lcd.print("Sun ");
      break;
  }

  SerialUSB.print(rtc[4], DEC);
  lcd.print(rtc[4], DEC);

  SerialUSB.print(" ");
  lcd.print(" ");

  switch (rtc[5]) {
    case 1:
      SerialUSB.print("jan");
      lcd.print("Jan ");
      break;
    case 2:
      SerialUSB.print("feb");
      lcd.print("Feb ");
      break;
    case 3:
      SerialUSB.print("mar");
      lcd.print("Mar ");
      break;
    case 4:
      SerialUSB.print("apr");
      lcd.print("Apr ");
      break;
    case 5:
      SerialUSB.print("may");
      lcd.print("May ");
      break;
    case 6:
      SerialUSB.print("jun");
      lcd.print("Jun ");
      break;
    case 7:
      SerialUSB.print("jul");
      lcd.print("Jul ");
      break;
    case 8:
      SerialUSB.print("aug");
      lcd.print("Aug ");
      break;
    case 9:
      SerialUSB.print("sep");
      lcd.print("Sep ");
      break;
    case 10:
      SerialUSB.print("oct");
      lcd.print("Oct ");
      break;
    case 11:
      SerialUSB.print("nov");
      lcd.print("Nov ");
      break;
    case 12:
      SerialUSB.print("dec");
      lcd.print("Dec ");
      break;
  }
  SerialUSB.print(" ");

  SerialUSB.println(rtc[6], DEC);
  lcd.print(rtc[6], DEC);

  lcd.setCursor(0, 5);
  if (rtc[2] < 10) {
    SerialUSB.print("0");
    lcd.print("0");
  }
  SerialUSB.print(rtc[2], DEC);
  lcd.print(rtc[2], DEC);
  SerialUSB.print(":");
  lcd.print(":");

  if (rtc[1] < 10) {
    SerialUSB.print("0");
    lcd.print("0");
  }
  SerialUSB.print(rtc[1], DEC);
  lcd.print(rtc[1], DEC);
  SerialUSB.print(":");
  lcd.print(":");

  if (rtc[0] < 10) {
    SerialUSB.print("0");
    lcd.print("0");
  }
  SerialUSB.println(rtc[0], DEC);
  lcd.print(rtc[0], DEC);


  SerialUSB.println();
  delay(1000);
}
