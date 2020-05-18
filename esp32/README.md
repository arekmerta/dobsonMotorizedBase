# What is it?
This is ESP32 part of the motorized dobson stand. I'm using ESP32 Wroom module.

# Full description?
Project main url: http://uczymy.edu.pl/wp/podstawa-teleskopu-celestron-c90-mak-typu-dobson/

# Derived from?
This project is using: 
* https://github.com/espressif/arduino-esp32
* https://circuits4you.com/2018/12/31/esp32-hardware-serial2-example/
* https://github.com/johnrickman/LiquidCrystal_I2C
* https://github.com/adafruit/Adafruit_GPS

# Environment
Use with Arduino IDE.

# Configuration
Board: ESP32 Dev Module
Upload speed: 921600
CPU Frequecy: 240 MHz
Flash Frequency: 80 MHz
Flash Mode: QIO
Partition Scheme: Hudge App
PSRAM: disabled

# What's there?
ESP32 is there to:
* read values from Nordic Thingy:52 (like heading and yaw) over BLE, 
* read GPS position from an UART GPS module, using gpio 17 for TX and 16 for RX, 
* display values on a attached LCD screen, connected to i2c
* use joystick as keyboard, with arrows + Ok button, connected to gpio 34, 35 for x/y axis and gpio 32 for the button, used with keyes_sjoys type. 
