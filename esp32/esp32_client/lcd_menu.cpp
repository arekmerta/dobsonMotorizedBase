//lcd menu class - creating a simple menu on LCD 4x16 display
//
//For more on my dobson stand go to: http://uczymy.edu.pl/wp/podstawa-teleskopu-celestron-c90-mak-typu-dobson/

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "defs.h"

#define lcd_print(row, col, ...)lcd.setCursor(row, col ); lcd.print(__VA_ARGS__)

/* Menu item structure
 * Fields:
 * char menuId: Id of the menu - must be unique number and > 100
 * menuType: One of the following:
 *     MENU_TYPE_HEADING - the top heading
 *     MENU_TYPE_SUBITEM - item leading to next menu
 *     MENU_TYPE_ACTION - final item ending with action
 *     MENU_TYPE_VALUE - item storing a value
 * char menuChildOfId: this menu item is a child of menuId item
 * String menuString: what is diaplyed on the screen - aligned to row length
 * char menuPushId: what is returned in case element is MENU_TYPE_ACTION
 * int minValue: minimum value of the item
 * int maxValue: maximum value of the item
 * int currentValue: alue set by the user
 */
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


//The class
class LcdMenu{
private:
  LiquidCrystal_I2C *_lcd;
  tMenuItem *_menus;
  int _lastItemRow = 1;
  char _maxMenuRow = 3;
  char _displayedMenuIds[4];
  char _noActivityId = 0;    
  
  void updateRowSelection(char row ){
    _lcd->setCursor(0,_lastItemRow);
    _lcd->print(" ");
    
    _lcd->setCursor(0,row);
    _lcd->print(">");

    _lastItemRow = row;
  }

  void displayItemAtRow(int row, tMenuItem* menuItem, boolean selected = false){
    _displayedMenuIds[row] = menuItem->menuId;
    _lcd->setCursor(0,row);
    _lcd->print(menuItem->menuString);

    if( selected ){
      _lcd->setCursor(0,row);
      _lcd->print(">");
  
    }

    if( menuItem->menuType == MENU_TYPE_VALUE ){
      _lcd->setCursor(LCD_COLUMNS-3, row);
      if( menuItem->currentValue < 10 ){
        _lcd->print("_");
      }
      if( menuItem->currentValue < 100 ){
        _lcd->print("_");
      }
      _lcd->print( menuItem->currentValue );
    }
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
  //lcd: Init with lcd ointer and structure of menus
  //menus: array of menus
  //noActivityId: id that will be called after 5 seconds of beeing idle,
  LcdMenu(LiquidCrystal_I2C *lcd, tMenuItem *menus, char noActivityId){
    _lcd = lcd;
    _menus = menus;
    _noActivityId = noActivityId;
  }

  //display menu of id menuId
  //returns false if menuId was not found or not type of MENU_TYPE_HEADING
  boolean displayMenu(char menuId){
    int i=0;
    int row = 0;
    boolean found = false;
    _lcd->clear();
    while( _menus[i].menuType != MENU_TYPE_LAST ){
      
      if( _menus[i].menuId == menuId && _menus[i].menuType == MENU_TYPE_HEADING ){
        //this is heading, display at top
        displayItemAtRow(row++, &_menus[i]);
        found = true;
      }else if( _menus[i].menuChildOfId == menuId && _menus[i].menuType != MENU_TYPE_HEADING ){
        //this is menu item
        displayItemAtRow(row++, &_menus[i]);
      }
      i = i+1;
    }
    if( !found ) return false;
    _maxMenuRow = row - 1;
    _lastItemRow = 1;
    updateRowSelection(1);
    return true;
  }

  //Loop function - call it with button pressed
  int loop(char button){
    static int32_t lastActivity = millis();
    
    if( button != BUTTON_NOARROW ){
      lastActivity = millis();  
    }

    if( (millis() - lastActivity) > SECONDS_5 ){
      return _noActivityId;
    }
    
    if( button == BUTTON_UP ){
      updateRowSelection( _lastItemRow > 1?_lastItemRow-1:1 );
    }
    if( button == BUTTON_DOWN ){
      updateRowSelection( _lastItemRow < _maxMenuRow?_lastItemRow+1:_maxMenuRow );
    }
    if( button == BUTTON_RIGHT ){
      //Check what kind of menu is selected
      tMenuItem* mi = getMenuItemById( _displayedMenuIds[_lastItemRow] );
      if( mi != NULL && mi->menuType == MENU_TYPE_VALUE ){
        mi->currentValue = mi->currentValue > mi->minValue?mi->currentValue-1:mi->minValue;
        displayItemAtRow(_lastItemRow, mi, true);
      }
    }
    if( button == BUTTON_LEFT ){
      //Check what kind of menu is selected
      tMenuItem* mi = getMenuItemById( _displayedMenuIds[_lastItemRow] );
      if( mi != NULL && mi->menuType == MENU_TYPE_VALUE ){
        mi->currentValue = mi->currentValue < mi->maxValue?mi->currentValue+1:mi->maxValue;
        displayItemAtRow(_lastItemRow, mi, true);
      }
    }
    if( button == BUTTON_OK ){
      //Check what kind of menu is selected
      tMenuItem* mi = getMenuItemById( _displayedMenuIds[_lastItemRow] );
      if( mi != NULL ){
        //open a submenu
        if( mi-> menuType == MENU_TYPE_SUBITEM ){
          displayMenu( mi->menuPushId );  
          return mi->menuPushId;
        }
        //action-item clicked
        if( mi-> menuType == MENU_TYPE_ACTION ){
          return mi->menuId;  
        }
        //changing value of an item
        if( mi-> menuType == MENU_TYPE_VALUE ){
          
        }
       }
    }
  }
};
