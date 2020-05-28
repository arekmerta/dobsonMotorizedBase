#ifndef __SCREEN_DISPLAY_H__
#define __SCREEN_DISPLAY_H__

#define LED_ON 1
#define LED_OFF 0

#define LED_ON 1
#define LED_OFF 0

struct A_PIN{
unsigned char _row;
unsigned char _col;
};

const A_PIN microbit_pins[5][5]={
{{26, 3}, {28,23}, {27, 4}, {26, 7}, {28,10}}, 
{{27,23}, {28,24}, {26, 6}, {26, 9}, {27, 9}},
{{26, 4}, {28,25}, {27,10}, {26,25}, {28, 3}}, 
{{27,24}, {28, 9}, {28, 6}, {26,24}, {27,25}},
{{26,10}, {28, 7}, {27, 3}, {26,23}, {28, 4}}
};

struct POWER_LINE{
  int _row;
  bool _up;
};

class ScreenDisplay{
  private:
    unsigned char _buffer[5][5];
    POWER_LINE powerLines[3]={{26, false},{27, false},{28, false}};

    void _drawPixel(int row, int col, int state);

    void _fillScreen(int state);

    void _clear();
    
  public:
    ScreenDisplay();
    
    void drawPixel(int row, int col, int state);
    
    void fillScreen(int state);
    
    void clear();

    void restore();

    void loop();
    
};
#endif
