//Keyes sjoy to keyboard
//
//For more on my dobson stand go to: http://uczymy.edu.pl/wp/podstawa-teleskopu-celestron-c90-mak-typu-dobson/

#include <Arduino.h>
#include "defs.h"

#define X_AXIS_GPIO 34
#define Y_AXIS_GPIO 35
#define BUTTON_GPIO 32

//Class for joystick working as keyboard

class Joystick{
  private:
    int _gpioBtn;
    int _limit;
    int _middle = 2000;

    boolean _buttonOKLock = false;
    boolean _buttonOKPressed = false;
    
    boolean _buttonArrowLock = false;
    int _buttonArrowPRESSED = 0;
    uint32_t _arrowMillis = 0;

  public:
    //gpioBtn: where Btn pin is connected
    //limit: percent what it means that stick has been moved, actually sensitivity
    Joystick(int gpioBtn, int limit){
      _gpioBtn = gpioBtn;
      _limit = limit;
      
      pinMode(gpioBtn, INPUT_PULLUP);
    }

    //True until button not read
    boolean getOK(){
      if( _buttonOKPressed ){
        _buttonOKPressed = false;
        return true;
      }
      return false;
    }

    int getArrow(){
      if( _buttonArrowPRESSED != BUTTON_NOARROW ){
        int _arrow = _buttonArrowPRESSED;
        _buttonArrowPRESSED = BUTTON_NOARROW;
        _arrowMillis = millis();
        return _arrow;
      }
      return BUTTON_NOARROW;
    }
    
    void loop(){
           
      int btn = digitalRead( BUTTON_GPIO );  

      //check if not pressed
      if( btn == 1 )_buttonOKLock = false;
      if( btn == 0 && !_buttonOKLock ){
        _buttonOKPressed = true;
        _buttonOKLock = true;
      }

      int x   =  analogRead( X_AXIS_GPIO );
      int y   =  analogRead( Y_AXIS_GPIO );
      int xX = diffProc(x, (float)_middle );
      int yY = diffProc(y, (float)_middle );

      if( xX < _limit && yY < _limit && ((millis() -_arrowMillis) > 250 ) ){
        _buttonArrowLock = false;
      }

      if( (xX > _limit || yY > _limit) && ! _buttonArrowLock ){
        if( xX > _limit ){
          if( x < _middle )_buttonArrowPRESSED = BUTTON_LEFT;
          else _buttonArrowPRESSED = BUTTON_RIGHT;
          
        }else if( yY > _limit ){
          if( y < _middle )_buttonArrowPRESSED = BUTTON_UP;
          else _buttonArrowPRESSED = BUTTON_DOWN;   
        }
        _buttonArrowLock = true;
      }
    }   
};
