#include <Arduino.h>
#include "ScreenDisplay.h"

//https://github.com/adafruit/Adafruit_Microbit/blob/master/examples/timerdemo/timerdemo.ino
//https://forum.arduino.cc/index.php?topic=66670.0

void ScreenDisplay::loop(void) {
  static int _thisPowerLine = 0;
  _thisPowerLine+=1;
  if (_thisPowerLine > 2 )_thisPowerLine = 0;

  if( !powerLines[_thisPowerLine]._up )return;

  //switch other power lines  off
  for(int z=0;z<3;z++){
    digitalWrite( powerLines[z]._row, LOW );  
  }
  for(int k=0; k < 5; k++)
    for(int z = 0; z< 5; z++ ){
      digitalWrite( microbit_pins[k][z]._col, HIGH );
  }
  //now, turn on lines that supposed to be displayed and turn off ones that are dimm now
  for(int k=0; k < 5; k++)
    for(int z = 0; z< 5; z++ ){
      if( 
        (microbit_pins[k][z]._row == powerLines[_thisPowerLine]._row) && 
        _buffer[k][z] == 1 )
          digitalWrite( microbit_pins[k][z]._col, LOW );  
     }
  
  digitalWrite( powerLines[_thisPowerLine]._row, HIGH );  
}

void ScreenDisplay::_drawPixel(int row, int col, int state){

  int pl     = microbit_pins[row][col]._row;
  int pl_col = microbit_pins[row][col]._col;

  powerLines[pl-26]._up           = LED_ON?true:false;      

}

void ScreenDisplay::_fillScreen(int state){
  int rows = HIGH;
  int cols = LOW;
  
  if( state == LED_OFF ){
    rows = LOW;   
  }
  for(int i = 0; i < 5; i++)
    for(int z = 0; z < 5; z++){
      digitalWrite( microbit_pins[i][z]._row, rows );
      digitalWrite( microbit_pins[i][z]._col, cols );
    }
 
}

void ScreenDisplay::_clear(){
  _fillScreen(LED_OFF);
}

ScreenDisplay::ScreenDisplay(){

  for(int i=0; i<5; i++)
    for(int z=0; z<5; z++){
      pinMode( microbit_pins[i][z]._row, OUTPUT );
      pinMode( microbit_pins[i][z]._col, OUTPUT );
    }
  
}
    
void ScreenDisplay::drawPixel(int row, int col, int state){
  if (state != _buffer[row][col] ){
    _buffer[row][col]=state;
    _drawPixel(row, col, state);
/*  
    Serial.print("ss.drawPixel( ");
    Serial.print(row);
    Serial.print(", ");
    Serial.print(col);
    Serial.print(", ");
    Serial.print(state);
    Serial.println(");");
    */
  }
  
}
    
void ScreenDisplay::fillScreen(int state){
  _fillScreen(state);
}
    
void ScreenDisplay::clear(){
  _clear();
}

void ScreenDisplay::restore(){
  for(int row = 0; row < 5; row++)
  for(int col = 0; col < 5; col++)
    _drawPixel(row, col, _buffer[row][col]);
}
