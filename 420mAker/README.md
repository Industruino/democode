# 4-20mA.ker demo sketches

For testing, the 'INDIO_show_ch1' sketch can be uploaded to the IND.I/O controller to show the analog input value on channel 1.  

Please note that the Arduino ```map()``` function only handles integers. To map floating value mA, use the helper function in the 'static-demo' sketch.
