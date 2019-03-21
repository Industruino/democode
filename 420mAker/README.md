# 4-20mA.ker demo sketches

For testing, the 'INDIO_show_ch1' sketch can be uploaded to the IND.I/O controller to show the analog input value on channel 1.  

Notes:
1. The Arduino ```map()``` function only handles integers. To map floating value mA, use the helper function in the 'static-demo' sketch:
```
// helper function to map milliamps to 0-4095 DAC range
int mAmap(float ma) {
  return map(1000 * ma, 3800, 20700, 0, 4095); // map microamps 3.8-20.7mA
}
```
Then you can use this to write any mA_value: ```analogWrite(PIN_DAC0, mAmap(mA_value));```
2. As long as the transmitter's USB is connected, the mA output values are not correct. Disconnect to get the real output.
3. Default serial configuration in the Arduino IDE *Tools > Board* is 'SPI+I2C' which is fine for the demo sketches; if you want need a UART please select the required option. 
