//Number of defines used in project
//
//For more on my dobson stand go to: http://uczymy.edu.pl/wp/podstawa-teleskopu-celestron-c90-mak-typu-dobson/

#define lcd_print(row, col, ...)lcd.setCursor(row, col ); lcd.print(__VA_ARGS__)

#define SECONDS_1 1000
#define SECONDS_5 5000
#define SECONDS_10 10000
#define SECONDS_30 30000

//Logging
//#define LOG_LEVEL_INFO_VERBOSE
#define LOG_LEVEL_INFO

#if defined(LOG_LEVEL_INFO_VERBOSE)
  #define log_v(...) Serial.print(__VA_ARGS__)
  #define log_vln(...) Serial.println(__VA_ARGS__)
  #define log_i(...) Serial.print(__VA_ARGS__)
  #define log_iln(...) Serial.println(__VA_ARGS__)
#elif defined(LOG_LEVEL_INFO)
  #define log_v(...) 
  #define log_vln(...) 
  #define log_i(...) Serial.print(__VA_ARGS__)
  #define log_iln(...) Serial.println(__VA_ARGS__)
#else
  #define log_v(...) 
  #define log_vln(...) 
  #define log_i(...) 
  #define log_iln(...) 
#endif

#define diffProc(a,b) (int)(abs((float)(a-b)/(float)b)*100.)


#define BUTTON_UP    100
#define BUTTON_DOWN  101
#define BUTTON_OK    102
#define BUTTON_LEFT  103
#define BUTTON_RIGHT 104
#define BUTTON_NOARROW 0

#define LCD_COLUMNS 16 
#define LCD_ROWS 4
