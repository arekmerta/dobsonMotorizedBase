#include <Arduino.h>

/*
 * Class provides functionality of light sensor
 * Based on: https://lancaster-university.github.io/microbit-docs/extras/light-sensing/
 */
class LightSensor{
  private:
    
  public:
    LightSensor(){
    }

    //Return light level
    int measure(){
      const int rows[]={26,27,28};
      const int cols[]={3,4,10};
      int _ret = 0;
      for(int channel = 0; channel < 3; channel++){
        pinMode(rows[channel], OUTPUT);
        pinMode(cols[channel], OUTPUT);
        digitalWrite(rows[channel], LOW);
        digitalWrite(cols[channel], LOW);
      }
      delay(10);
      for(int channel = 0; channel < 3; channel++){
        digitalWrite(cols[channel], HIGH);
        pinMode(cols[channel], INPUT);
        delay(15);
        int val = analogRead(cols[channel]);
        _ret += val;
      }
      //Restore modes
      for(int channel = 0; channel < 3; channel++){
        pinMode(cols[channel], OUTPUT);
      }   
      _ret = _ret / 3;

      return _ret;
    }
 };
