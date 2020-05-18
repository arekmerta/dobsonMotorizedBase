# dobsonMotorizedBase
This site contains source code for my Dobson motorized telescope base. It was originally designed for Celestron C90 Mac and Canon 450D/Rebel DSLR camera, but can be adapted (or used directly) for similar setups. 

You can find the whole project description here (in Polish):
[Podstawa Teleskopu](http://uczymy.edu.pl/wp/podstawa-teleskopu-celestron-c90-mak-typu-dobson/)

The project has been splint into number of phases:
* Mark I - the base is moved manually, no code/electronics needed,
* Mark II - the base position is controlled and displayed on a LCD, esp32 part
* Mark III: the base X and Y axis is motorized, Arduino with CNC part,
* Mark IV: user can select an object on the sky - and the telescope tube will be moved to this position, esp32+communication with Arduino
* Mark V: support for astrophotography, esp32

## Notes  
* The 'arduino' dir contains code for arduino and CNC shield - driving base motors
* The 'esp32' dir contains code for esp32 - driving getting position from thingy:52 sensors, driving LCD, being controlled by a joystick
* The 'experimental' dir contains some additional code - not diectly used for project (e.g. remote shutter for canon eos with micro:bit over BLE)  

## Progress:

Date|Phase|Achievement
----|-----|-----------
May, 2020|Mark I|Completed
May, 2020|Mark II| Developing code for the ESP32 (reading Thingy:52 heading and yaw, reading GPS/UART, LCD display on I2C, analog joystick)
