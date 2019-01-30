/*
  Industruino 4-20mA.ker test sketch for MPU6050
  this sketch:
  > sends 20mA on startup
  > reads pitch and roll
  > maps pitch to 4-16mA, 10mA = horizontal pitch=0
  > if data not available (not-a-number), send 18mA
  > if sensor reading gets stuck, WDT resets the unit

  Connections on 4-20mA.ker to MPU6050:
  1     NC
  2     NC
  3     NC
  4     NC
  5     SDA
  6     SCL
  7     GND
  8     VCC
*/

  
#include <Wire.h>
#include "Kalman.h" // Source: https://github.com/TKJElectronics/KalmanFilter
uint8_t i2cData[14]; // Buffer for I2C data
uint32_t timer;

#define RESTRICT_PITCH // Comment out to restrict roll to ±90deg instead - please read: http://www.freescale.com/files/sensors/doc/app_note/AN3461.pdf

Kalman kalmanX; // Create the Kalman instances
Kalman kalmanY;

/* IMU Data */
int16_t accX, accY, accZ;
int16_t gyroX, gyroY, gyroZ;
int16_t tempRaw;

float gyroXangle, gyroYangle; // Angle calculate using the gyro only
float compAngleX, compAngleY; // Calculated angle using a complementary filter
float kalAngleX, kalAngleY; // Calculated angle using a Kalman filter

float roll;
float pitch;

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
  SerialUSB.println("4-20mA.ker MPU6050 test");

  DAC->CTRLB.bit.REFSEL = 0x00; //Set DAC to external VREF (2.5V)
  analogWriteResolution(12);
  digitalWrite(PIN_EXT_3V3_ENABLE, HIGH);
  analogWrite(PIN_DAC0, uAmap(20000));      // send 20mA at startup
  SerialUSB.println("output 20mA at startup");
  
//  delay(5000);  // to allow upload before WDT is enabled
  
  WDT->CTRLA.reg = 0;                       // init WDT
  WDT->CONFIG.reg = WDT_CONFIG_PER_CYC1024; // set timeout period based on 1.024kHz clock, from 8ms to 16sec, CYC = 1ms
          // options: WDT_CONFIG_PER_CYC8 WDT_CONFIG_PER_CYC16 WDT_CONFIG_PER_CYC32 WDT_CONFIG_PER_CYC64 WDT_CONFIG_PER_CYC128
          // WDT_CONFIG_PER_CYC256 WDT_CONFIG_PER_CYC512 WDT_CONFIG_PER_CYC1024 WDT_CONFIG_PER_CYC2048 WDT_CONFIG_PER_CYC4096
          // WDT_CONFIG_PER_CYC8192 WDT_CONFIG_PER_CYC16384

  WDT->CTRLA.reg = WDT_CTRLA_ENABLE;        // enable WDT

  timestamp = millis();

  initIMU();

}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void loop() {

  WDT->CLEAR.reg = WDT_CLEAR_CLEAR_KEY;   // reset the WDT 
  
  getIMU();

  if (isnan(pitch) || isnan(roll)) {             // if no data

    SerialUSB.print("sensor problem, output 18mA");
    analogWrite(PIN_DAC0, uAmap(18000));
    initIMU();

  } else {

    SerialUSB.print("pitch:\t");
    SerialUSB.print(pitch);
    SerialUSB.print("* \t");
    SerialUSB.print("roll:\t");
    SerialUSB.print(roll);
    SerialUSB.print("* \t");

    int microamps = map(pitch, -90, 90, 4000, 16000);           // MAPPING
    //if (isnan(t)) microamps = 11000;
    SerialUSB.print(microamps / 1000.0);
    SerialUSB.println("mA \t");
    analogWrite(PIN_DAC0, uAmap(microamps));

  }

  delay(10);

}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

void initIMU() {
  Wire.begin();
  Wire.setClock(400000);
  //TWBR = ((F_CPU / 400000L) - 16) / 2; // Set I2C frequency to 400kHz

  i2cData[0] = 7; // Set the sample rate to 1000Hz - 8kHz/(7+1) = 1000Hz
  i2cData[1] = 0x00; // Disable FSYNC and set 260 Hz Acc filtering, 256 Hz Gyro filtering, 8 KHz sampling
  i2cData[2] = 0x00; // Set Gyro Full Scale Range to ±250deg/s
  i2cData[3] = 0x00; // Set Accelerometer Full Scale Range to ±2g
  while (i2cWrite(0x19, i2cData, 4, false)); // Write to all four registers at once
  while (i2cWrite(0x6B, 0x01, true)); // PLL with X axis gyroscope reference and disable sleep mode

  while (i2cRead(0x75, i2cData, 1));
  if (i2cData[0] != 0x68) { // Read "WHO_AM_I" register
    SerialUSB.print(F("Error reading sensor"));
    while (1);
  }

  delay(100); // Wait for sensor to stabilize

  /* Set kalman and gyro starting angle */
  while (i2cRead(0x3B, i2cData, 6));
  accX = (i2cData[0] << 8) | i2cData[1];
  accY = (i2cData[2] << 8) | i2cData[3];
  accZ = (i2cData[4] << 8) | i2cData[5];

  // Source: http://www.freescale.com/files/sensors/doc/app_note/AN3461.pdf eq. 25 and eq. 26
  // atan2 outputs the value of -π to π (radians) - see http://en.wikipedia.org/wiki/Atan2
  // It is then converted from radians to degrees
#ifdef RESTRICT_PITCH // Eq. 25 and 26
  roll  = atan2(accY, accZ) * RAD_TO_DEG;
  pitch = atan(-accX / sqrt(accY * accY + accZ * accZ)) * RAD_TO_DEG;
#else // Eq. 28 and 29
  roll  = atan(accY / sqrt(accX * accX + accZ * accZ)) * RAD_TO_DEG;
  pitch = atan2(-accX, accZ) * RAD_TO_DEG;
#endif

  kalmanX.setAngle(roll); // Set starting angle
  kalmanY.setAngle(pitch);
  gyroXangle = roll;
  gyroYangle = pitch;
  compAngleX = roll;
  compAngleY = pitch;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void getIMU() {
  /* Update all the values */
  while (i2cRead(0x3B, i2cData, 14));   // hangs here when sensor not present

  accX = ((i2cData[0] << 8) | i2cData[1]);
  accY = ((i2cData[2] << 8) | i2cData[3]);
  accZ = ((i2cData[4] << 8) | i2cData[5]);
  tempRaw = (i2cData[6] << 8) | i2cData[7];
  gyroX = (i2cData[8] << 8) | i2cData[9];
  gyroY = (i2cData[10] << 8) | i2cData[11];
  gyroZ = (i2cData[12] << 8) | i2cData[13];

  float dt = (float)(micros() - timer) / 1000000; // Calculate delta time
  timer = micros();

  // Source: http://www.freescale.com/files/sensors/doc/app_note/AN3461.pdf eq. 25 and eq. 26
  // atan2 outputs the value of -π to π (radians) - see http://en.wikipedia.org/wiki/Atan2
  // It is then converted from radians to degrees
#ifdef RESTRICT_PITCH // Eq. 25 and 26
  roll  = atan2(accY, accZ) * RAD_TO_DEG;
  pitch = atan(-accX / sqrt(accY * accY + accZ * accZ)) * RAD_TO_DEG;
#else // Eq. 28 and 29
  roll  = atan(accY / sqrt(accX * accX + accZ * accZ)) * RAD_TO_DEG;
  pitch = atan2(-accX, accZ) * RAD_TO_DEG;
#endif

  float gyroXrate = gyroX / 131.0; // Convert to deg/s
  float gyroYrate = gyroY / 131.0; // Convert to deg/s

#ifdef RESTRICT_PITCH
  // This fixes the transition problem when the accelerometer angle jumps between -180 and 180 degrees
  if ((roll < -90 && kalAngleX > 90) || (roll > 90 && kalAngleX < -90)) {
    kalmanX.setAngle(roll);
    compAngleX = roll;
    kalAngleX = roll;
    gyroXangle = roll;
  } else
    kalAngleX = kalmanX.getAngle(roll, gyroXrate, dt); // Calculate the angle using a Kalman filter

  if (abs(kalAngleX) > 90)
    gyroYrate = -gyroYrate; // Invert rate, so it fits the restriced accelerometer reading
  kalAngleY = kalmanY.getAngle(pitch, gyroYrate, dt);
#else
  // This fixes the transition problem when the accelerometer angle jumps between -180 and 180 degrees
  if ((pitch < -90 && kalAngleY > 90) || (pitch > 90 && kalAngleY < -90)) {
    kalmanY.setAngle(pitch);
    compAngleY = pitch;
    kalAngleY = pitch;
    gyroYangle = pitch;
  } else
    kalAngleY = kalmanY.getAngle(pitch, gyroYrate, dt); // Calculate the angle using a Kalman filter

  if (abs(kalAngleY) > 90)
    gyroXrate = -gyroXrate; // Invert rate, so it fits the restriced accelerometer reading
  kalAngleX = kalmanX.getAngle(roll, gyroXrate, dt); // Calculate the angle using a Kalman filter
#endif

  gyroXangle += gyroXrate * dt; // Calculate gyro angle without any filter
  gyroYangle += gyroYrate * dt;
  //gyroXangle += kalmanX.getRate() * dt; // Calculate gyro angle using the unbiased rate
  //gyroYangle += kalmanY.getRate() * dt;

  compAngleX = 0.93 * (compAngleX + gyroXrate * dt) + 0.07 * roll; // Calculate the angle using a Complimentary filter
  compAngleY = 0.93 * (compAngleY + gyroYrate * dt) + 0.07 * pitch;

  // Reset the gyro angle when it has drifted too much
  if (gyroXangle < -180 || gyroXangle > 180)
    gyroXangle = kalAngleX;
  if (gyroYangle < -180 || gyroYangle > 180)
    gyroYangle = kalAngleY;

  /* Print Data */
#if 0 // Set to 1 to activate
  SerialUSB.print(accX); SerialUSB.print("\t");
  SerialUSB.print(accY); SerialUSB.print("\t");
  SerialUSB.print(accZ); SerialUSB.print("\t");

  SerialUSB.print(gyroX); SerialUSB.print("\t");
  SerialUSB.print(gyroY); SerialUSB.print("\t");
  SerialUSB.print(gyroZ); SerialUSB.print("\t");

  SerialUSB.print("\t");

  SerialUSB.print(roll); SerialUSB.print("\t");
  SerialUSB.print(gyroXangle); SerialUSB.print("\t");
  SerialUSB.print(compAngleX); SerialUSB.print("\t");
  SerialUSB.print(kalAngleX); SerialUSB.print("\t");

  SerialUSB.print("\t");

  SerialUSB.print(pitch); SerialUSB.print("\t");
  SerialUSB.print(gyroYangle); SerialUSB.print("\t");
  SerialUSB.print(compAngleY); SerialUSB.print("\t");
  SerialUSB.print(kalAngleY); SerialUSB.print("\t");

  SerialUSB.print("\t");

  double temperature = (double)tempRaw / 340.0 + 36.53;
  SerialUSB.print(temperature); SerialUSB.print("\t");
  SerialUSB.println();
#endif

}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// helper function to map milliamps to 0-4095 DAC range
int uAmap(int ua) {
  return map(ua, 3800, 20700, 0, 4095); // map microamps 3.8-20.7mA
}


