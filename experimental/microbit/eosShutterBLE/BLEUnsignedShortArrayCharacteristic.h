#ifndef _BLE_UNSIGNEDSHIRT__ARRAY_CHARACTERISTICS_H_
#define _BLE_UNSIGNEDSHIRT__ARRAY_CHARACTERISTICS_H_

#include "BLEFixedLengthCharacteristic.h"


class BLEUnsignedShortArrayCharacteristic : public BLEFixedLengthCharacteristic {
  public:
    BLEUnsignedShortArrayCharacteristic(const char* uuid, unsigned char properties, unsigned char valueSize);
   
    bool getUShortValues(unsigned short* arr);
    bool setUShortValues(unsigned short*arr);
    //bool setUShortValue(unsigned char index, unsigned short value);

    //virtual unsigned char valueSize() const;
};

#endif
