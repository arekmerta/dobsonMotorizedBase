/*
 * This characteristics uses array of usigned chars as value
 * uczymy.edu.pl on basis of https://github.com/melvyniandrag/arduino-BLEPeripheral/blob/master/src/BLECharArrayCharacteristic.cpp
 * 
 */
#include <Arduino.h>
#include "BLEUnsignedShortArrayCharacteristic.h"

BLEUnsignedShortArrayCharacteristic::BLEUnsignedShortArrayCharacteristic(const char* uuid, unsigned char properties, unsigned char valueSize) :
  BLEFixedLengthCharacteristic(uuid, properties, 2 * valueSize)
{
  this->_valueLength = valueSize;
}

/*
 * This is used to set a UShort value; needs to break UShort into two bytes and write in proper order
 * index: which number (not byte) is this
 * val: value to be written
 * notify: send on notification change (if true)
 */


bool BLEUnsignedShortArrayCharacteristic::setUShortValues(unsigned short*vals){

  static int pos = 0;
  unsigned char a1 = (vals[0] >> 8) & 0xFF;
  unsigned char a2 = (vals[0]) & 0xFF;
  unsigned char b1 = (vals[1] >> 8) & 0xFF;
  unsigned char b2 = (vals[1]) & 0xFF;
  
  unsigned char arrs[4]={a2,a1,b2,b1};
/*
  Serial.print("Setting output: ");
  Serial.print(a1,HEX);Serial.print(",");
  Serial.print(a2,HEX);Serial.print(",");
  Serial.print(b1,HEX);Serial.print(",");
  Serial.print(b2,HEX);Serial.println(",");
*/
  setValue(arrs, 4);

  return true;
}

/*
 * This is used to get a UShort value from the characteristics
 * arr: pointer to unsigned short array
 */
bool BLEUnsignedShortArrayCharacteristic::getUShortValues(unsigned short* arr){

  for(int i =0; i < valueSize(); i+=2){
    /*
    Serial.println(i);
    Serial.print("----i: ");
    Serial.println(value()[i]);
    Serial.print("----i+1: ");
    Serial.println(value()[i+1]);
    */
    unsigned short now = value()[i+1];
    now = (now<<8) & 0xFF00;
    now += value()[i];
    
    Serial.println(now);
    arr[i/2] = now;
  }
  return true;
}
