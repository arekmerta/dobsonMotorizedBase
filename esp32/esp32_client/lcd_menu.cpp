#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "defs.h"

#define lcd_print(row, col, ...)lcd.setCursor(row, col ); lcd.print(__VA_ARGS__)

struct tMenuItem{
  char menuId;
  char menuType;
  char menuChildOfId;
  String menuString;
  char menuPushId;
  int minValue;
  int maxValue;
  int currentValue;
};

//LcdMenu class defines
#define MENU_NOSIBLING    50
#define MENU_TYPE_HEADING 51
#define MENU_TYPE_SUBITEM 52
#define MENU_TYPE_ACTION  53
#define MENU_TYPE_VALUE   54
#define MENU_TYPE_LAST    55



class LcdMenu{
private:
  LiquidCrystal_I2C *_lcd;
  tMenuItem *_menus;
  int _lastItemRow = 1;
  char _maxMenuRow = 3;
  char _displayedMenuIds[4];
  
  void updateRowSelection(char row ){
    _lcd->setCursor(0,_lastItemRow);
    _lcd->print(" ");
    
    _lcd->setCursor(0,row);
    _lcd->print(">");

    _lastItemRow = row;
  }

  void displayItemAtRow(int row, tMenuItem* menuItem){
    _displayedMenuIds[row] = menuItem->menuId;
    _lcd->setCursor(0,row);
    _lcd->print(menuItem->menuString);
  }

   tMenuItem* getMenuItemById(int id){
    int i = 0;
    while( _menus[i].menuType != MENU_TYPE_LAST ){
      if( _menus[i].menuId == id )return &_menus[i];
      i = i + 1;
    }
    return NULL;
  }
public:
  //Init with lcd and structure of menus
  LcdMenu(LiquidCrystal_I2C *lcd, tMenuItem *menus){
    _lcd = lcd;
    _menus = menus;
  }

  boolean displayMenu(char menuId){
    int i=0;
    int row = 0;
    _lcd->clear();
    while( _menus[i].menuType != MENU_TYPE_LAST ){
      
      if( _menus[i].menuId == menuId && _menus[i].menuType == MENU_TYPE_HEADING ){
        //this is heading, display at top
        displayItemAtRow(row++, &_menus[i]);
        
      }else if( _menus[i].menuChildOfId == menuId && _menus[i].menuType != MENU_TYPE_HEADING ){
        //this is menu item
        displayItemAtRow(row++, &_menus[i]);
        
      }
      i = i+1;
    }
    _maxMenuRow = row - 1;
    _lastItemRow = 1;
    updateRowSelection(1);
  }
  
  int loop(char button){
    if( button == BUTTON_UP ){
      updateRowSelection( _lastItemRow > 1?_lastItemRow-1:1 );
    }
    if( button == BUTTON_DOWN ){
      updateRowSelection( _lastItemRow < _maxMenuRow?_lastItemRow+1:_maxMenuRow );
    }
    if( button == BUTTON_LEFT ){
    }
    if( button == BUTTON_RIGHT ){
    }
    if( button == BUTTON_OK ){
      //Check what kind of menu is selected
      tMenuItem* mi = getMenuItemById( _displayedMenuIds[_lastItemRow] );
      if( mi != NULL ){
        if( mi-> menuType == MENU_TYPE_SUBITEM ){
          displayMenu( mi->menuPushId );  
          return mi->menuPushId;
        }
        if( mi-> menuType == MENU_TYPE_ACTION ){
          return mi->menuId;  
        }
        if( mi-> menuType == MENU_TYPE_VALUE ){
          
        }
       }
    }
  }
};
