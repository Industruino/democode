# 4-20mA.ker demo sketches

For testing, the 'INDIO_show_ch1' sketch can be uploaded to the IND.I/O controller to show the analog input value on channel 1.  

Notes:
1. The Arduino ```map()``` function only handles integers. To map floating value mA, use the helper function in the 'static-demo' sketch.
2. As long as the transmitter's USB is connected, the mA output values are not correct. Disconnect to get the real output.
3. Default serial configuration in the Arduino IDE *Tools > Board* is 'SPI+I2C' which is fine for the demo sketches; if you want need a UART please select the required option. 
