This is ESP32 part of the motorized dobson stand.
Project main url: http://uczymy.edu.pl/wp/podstawa-teleskopu-celestron-c90-mak-typu-dobson/

ESP32 is there to read values from Nordic Thingy:52 (like heading and yaw), read GPS position from an UART blox GPS module, display values on a attached LCD screen (i2c). 

This Arduino IDE project is using: 

https://github.com/espressif/arduino-esp32

https://circuits4you.com/2018/12/31/esp32-hardware-serial2-example/

https://github.com/johnrickman/LiquidCrystal_I2C

https://github.com/adafruit/Adafruit_GPS
