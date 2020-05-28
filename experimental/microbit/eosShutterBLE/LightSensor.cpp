#include <Arduino.h>

/*
 * Class provides functionality of light sensor
 * Based on: 
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
        /*
        Serial.print("Got light:");
        Serial.println(_ret);
        
        if( _lastMeasure == -1 ){
          _lastMeasure = _ret;
          *ret = 0;
          return false;
        }else {
          int diff = diffProc( _ret, _lastMeasure);
          if( 
            ((diff > 0) && (diff > _sensitivityProc )) || 
            ((diff < 0) && (diff < (-1 * _sensitivityProc) )) ){
              //Act
              _lastMeasure = _ret;  
              *ret = diff;

              return true;
            }
        }
        return false;
        */
      }
   

};
/*
void onLightTick(int tick){
  Serial.print("Got tick: ");
  Serial.print(tick);
  Serial.println("%");
}

LightSensor ls(10, onLightTick);

void setup() {
  Serial.begin(9600);
  Serial.println("--------------");
}

void loop() {
  ls.loop();
}
*/
