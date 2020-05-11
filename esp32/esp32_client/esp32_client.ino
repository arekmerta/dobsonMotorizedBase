//uczymy.edu.pl: based on multiple examples (list may not be complete)
//Based on BLE_Client example from https://github.com/espressif/arduino-esp32
//     author unknown
//     updated by chegewara
//
//Based on LCD library from: https://github.com/johnrickman/LiquidCrystal_I2C
//Based on ESP32 Hardware Serial2 Example from https://circuits4you.com/2018/12/31/esp32-hardware-serial2-example/
//Based on Adafruit example GPS_HardwareSerial from https://github.com/adafruit/Adafruit_GPS
//

#include <stdio.h> // for function sprintf
#include "BLEDevice.h"
#include <LiquidCrystal_I2C.h>
#include <Adafruit_GPS.h>

#include "joystick.cpp"
//Joystick
Joystick stick(BUTTON_GPIO, 20);

//GPS
#define RXD2 16
#define TXD2 17
Adafruit_GPS GPS(&Serial2);

//Photo
static int shutterDelay = 1;
static int shutterSeries = 1;

//LCD
#define LCD_COLUMNS 16 
#define LCD_ROWS 4

LiquidCrystal_I2C lcd(0x38, LCD_COLUMNS, LCD_ROWS);

//Thingy MAC address
static BLEAddress thingyAddress("e3:f2:ca:0b:88:1e");

// The motion remote service 
static BLEUUID serviceUUID("EF680400-9B35-4933-9B10-52FFA9740042");
// The heading characteristic
static BLEUUID    charUUIDHeading("EF680409-9B35-4933-9B10-52FFA9740042");

// The euler angles characteristic
static BLEUUID    charUUIDEuler("EF680407-9B35-4933-9B10-52FFA9740042");

#define NO_OF_CHARACTERISTICS 2
static BLEUUID myCharacteristics[NO_OF_CHARACTERISTICS] = {charUUIDHeading, charUUIDEuler };

static volatile boolean connected = false;
static volatile boolean wasConnected = false;
static volatile float heading, yaw; 
static volatile bool newHeading = false, newYaw = false;

static BLEClient*  pClient;

#define SECONDS_1 1000
#define SECONDS_5 5000
#define SECONDS_10 10000
#define SECONDS_30 30000

//Canvas
#define CANVAS_POSITION 1
#define CANVAS_MENU 2
#define CANVAS_SHUTTER  3

static volatile int canvas = CANVAS_POSITION;
//**********************************************
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

//**********************************************
//Characteristics data conversions

String deg_to_dms(float deg){
  bool negative = deg < 0;
  log_vln(">>>> deg_to_dms");
  log_vln(deg,4);
  deg = abs(deg);
  int d = int(deg);
  log_v("d: ");
  log_vln(d);
  deg = (deg - d)*60.;
  log_v("b: ");
  log_vln(deg,4);
  int m = int(deg);
  log_v("m: ");
  log_vln(m);
  deg = (deg - m )*60.;
  int ss = int(deg);
  log_v("deg: ");
  log_vln(deg,4);
  char tbs[18];
  sprintf(tbs, "%c%3ds %2dm %2dss", negative?'-':' ', d, m, ss);
  log_v("Result: ");
  log_vln(tbs);
  log_vln("<<<< deg_to_dms");
  return String(tbs);
}
float bin2float(uint8_t* pData){
  log_vln(">>>> bin2float");
  log_v("------Received: 0x");
  log_v(pData[0], HEX);
  log_v(", 0x");
  log_v(pData[1], HEX);
  log_v(", 0x");
  log_v(pData[2], HEX);
  log_v(", 0x");
  log_v(pData[3], HEX);
  log_vln(".");
  bool negative = false;
  if (pData[0] & 0x80 ){
    negative = true;
    log_vln("Negative");
  }
  
  int32_t result = 0;
  result = pData[3];
  result = result << 8;
  result = result+pData[2];
  result = result << 8;
  result = result+pData[1];
  result = result << 8;
  result = result+pData[0];
  
  log_vln("int32: 0x");
  log_vln(result, HEX);
  
  float resultF = 0.;
  if(negative){
    result = (1 << 32) - 1 - result;
    result = result+1;
    resultF = -1. * (float(result) / 65535.);
  }
  else
    resultF = float(result) / 65535.;
  log_vln(resultF, 4);
  log_vln("<<<< bin2float");
  
  return resultF;
}

//**********************************************
//Notification callback
static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) 
  {
    
    log_vln(">>>> notifyCallback");
    log_vln(pBLERemoteCharacteristic->getUUID().toString().c_str());

    if( pBLERemoteCharacteristic->getUUID().equals(charUUIDHeading) && length == 4 && !newHeading )
    {
        float val = bin2float(pData);
        log_i("Heading: ");
        log_iln(val);
        heading = val;
        newHeading = true;
     }else if( pBLERemoteCharacteristic->getUUID().equals(charUUIDEuler) && length == 12 && !newYaw){      
        
        float val = bin2float(pData + 4);
        log_i("Yaw   : ");
        log_iln(val);
        yaw = val;
        newYaw = true;     
      
      }else{
        log_iln("----Not recognized UUID");
      }
    
    //else{
    //    log_vln("Display busy");  
    //}
    log_vln("<<<< notifyCallback");
}

//**********************************************
//Client connection callback
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    log_iln("On disconnect");
  }
};

//**********************************************
//Connect to a characteristics
bool connectToCharacteristics(BLERemoteService* pRemoteService, BLEUUID uuid){
  BLERemoteCharacteristic* pRemoteCharacteristic = pRemoteService->getCharacteristic(uuid);
  if (pRemoteCharacteristic == nullptr) {
    log_i("--->Failed to find our characteristic UUID: ");
    log_iln(uuid.toString().c_str());
    return false;
  }
  log_iln(" - Found");
  if(pRemoteCharacteristic->canNotify()){
    log_i("--->Registering for notifications...");
    pRemoteCharacteristic->registerForNotify(notifyCallback);
    log_iln("...done");
  }
}

//**********************************************
//Connect to a remote BLE device
bool connectToServer(BLEClient*  pClient) {
  log_i("Connecting to thingy...");
  if( pClient->connect( thingyAddress, BLE_ADDR_TYPE_RANDOM ) ){
    log_iln(" - Connected");
  }else {
    log_iln(" - ERROR");
    return false;
  }

  log_i("Connecting to service...");
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    log_i("Failed to find our service UUID: ");
    log_iln(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }  
  log_iln(" - found");
  
  log_i("Connecting to characteristics...");
  for(int i = 0; i < NO_OF_CHARACTERISTICS; i++){
    log_i("Connecting to characteristics ");
    log_i(i+1);
    log_i(" (of ");
    log_i(NO_OF_CHARACTERISTICS);
    log_i("), uuid: ");
    log_iln(myCharacteristics[i].toString().c_str());
    if( !connectToCharacteristics(
      pRemoteService,
      myCharacteristics[i]
      )){
        return false;
      }
  }
  return true;
}
//**********************************************
//Setup
void setup() {
  Serial.begin(115200);
  
  log_iln("Starting LCD");
  lcd.init(); 
  lcd.backlight();
  lcd.setCursor(0, 0); 
  lcd.print("Init...");
    
  log_iln("Starting Arduino BLE Client application...");
  BLEDevice::init("");
  pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());
  
  // Connect to the remove BLE Server.
  if( !(connected = connectToServer(pClient))){
    pClient->disconnect();
  }else{
    lcd.setCursor(0, 0); 
    lcd.print("Connected.   ");
    wasConnected = true;
  }

  log_iln("Starting GPS...");
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  
} 
//**********************************************
void lcd_print(int row, int col, String txt){
  lcd.setCursor(row, col );
  lcd.print(txt);
}

void canvas_position(){
static float lastHeading = 0., lastYaw = 0.;
  static float MAX_DIFF = 0.1;//%
  static uint32_t gpsTimer = millis() - 10000;
  static uint32_t thingyDisconnectTimer = millis();
    
  if( !connected && wasConnected) {
    log_iln("---->DISCONNECTED");
    
    lcd_print(0, 0, "Disconnected.   ");
    
    lcd_print(0, 1, "                ");
    
    pClient->disconnect();
    thingyDisconnectTimer = millis();
    wasConnected = false;
  }
    
  if ( !connected && !wasConnected && ((millis() - thingyDisconnectTimer ) > SECONDS_30)){
    //connect
    connected = connectToServer(pClient);

    if( connected ){
      lcd_print( 0, 0, "Connected.   ");
      wasConnected = true;
    }
    thingyDisconnectTimer = millis();
    
  }
  
  if( newHeading ){

    log_iln(diffProc(lastHeading, heading));
    if( diffProc(lastHeading, heading) > MAX_DIFF ) {
      String sHeading = deg_to_dms(heading);
      lcd_print(0, 0, "H:");
      lcd_print(2, 0, sHeading);
      
      for(int i=sHeading.length();i<16; i++)lcd.print(' ');
    }
    newHeading = false;
    lastHeading = heading;
  }

  if( newYaw ){
    if( diffProc(lastYaw,     yaw)     > MAX_DIFF ) {
      String sYaw     = deg_to_dms(yaw);
     
      lcd_print(0, 1, "Y:");
      lcd_print(2, 1, sYaw);
      for(int i=sYaw.length();i<16; i++)lcd.print(' ');
    }
    newYaw = false;
    lastYaw = yaw;
  }
  char c = GPS.read();
  if (GPS.newNMEAreceived()) {
    log_vln( GPS.lastNMEA() ); 
    if (!GPS.parse(GPS.lastNMEA())) return; 
  }
  if (GPS.fix && (millis() - gpsTimer > SECONDS_5) ) {
    gpsTimer = millis();
    log_i(GPS.latitude, 4); log_i(GPS.lat);
    log_i(", ");
    log_i(GPS.longitude, 4); log_iln(GPS.lon);
    log_i("Altitude: "); log_iln(GPS.altitude);

    lcd.setCursor(0, 2); 
    lcd.print( GPS.lat );
    lcd.print( " " );
    lcd.print( GPS.latitude );
      
    lcd.setCursor(0, 3); 
    lcd.print( GPS.lon );
    lcd.print( " " );
    lcd.print( GPS.longitude);

    //Date and time
    lcd.setCursor(11, 2); 
    if(  GPS.day < 10 )lcd.print('0');
    lcd.print(GPS.day, DEC); lcd.print('/');
    if(  GPS.month < 10 )lcd.print('0');
    lcd.print(GPS.month, DEC); 

    //Date and time
    lcd.setCursor(11, 3);
    if(  GPS.hour+2 < 10 )lcd.print('0');
    lcd.print(GPS.hour+2, DEC); lcd.print(':');
    if(  GPS.minute < 10 )lcd.print('0');
    lcd.print(GPS.minute, DEC); 
  }else if (!GPS.fix){
    lcd_print(0, 2, "No GPS fix yet  " );
    lcd_print(0, 3, "                ");
  }  
}
//**********************************************
#define MENU_MAIN 0
#define MENU_SHUTTER 1
#define MENU_POSITION 2

//which_menu, which_position, new_menu
void canvas_settings(char button){
  static int menuNow = MENU_MAIN;
  static int menuItem = 1, lastMenuItem = 1;
  static boolean refresh = true;
  static int maxSubitems = 2;
  
  if( menuNow == MENU_MAIN ){
    lcd_print(0, 0, "Main menu     " );
    lcd_print(2, 1, "Position      " );
    lcd_print(2, 2, "Shutter       " );
  }else if( menuNow == MENU_SHUTTER ){
    lcd_print(0, 0, "Shutter menu  " );
    lcd_print(2, 1, "Delay(s)      " );
    lcd_print(2, 2, "Series(n)     " );
    lcd_print(2, 3, "Dark Series(n)" );
  }else if ( menuNow == MENU_POSITION ){
    //not a real menu, display canvas
    canvas = CANVAS_POSITION;
    return;
  }

  if( lastMenuItem != menuItem ){
    lcd.setCursor(0, lastMenuItem );
    lcd.print(" ");
    lastMenuItem = menuItem;
  }

  lcd.setCursor(0, menuItem );
  lcd.print(">");


  if( button == BUTTON_UP ){
    menuItem = menuItem > 1?menuItem-1:1;
  }
  if( button == BUTTON_DOWN ){
    menuItem = menuItem < maxSubitems?menuItem+1:maxSubitems;
  }
  if( button == BUTTON_LEFT ){
    if(menuNow == MENU_SHUTTER && menuItem == 1 )
      shutterDelay = shutterDelay>1?shutterDelay-1:1;

    if(menuNow == MENU_SHUTTER && menuItem == 2 )
      shutterSeries = shutterSeries>1?shutterSeries-1:1;
  }
  if( button == BUTTON_RIGHT ){
    if(menuNow == MENU_SHUTTER && menuItem == 1 )
      shutterDelay = shutterDelay>1?shutterDelay-1:1;
      
    if(menuNow == MENU_SHUTTER && menuItem == 2 )
      shutterSeries = shutterSeries+1;
  }
  if ( button == BUTTON_OK ){
    if(menuNow == MENU_MAIN && menuItem == 1)
      menuNow = MENU_POSITION;
    if(menuNow == MENU_MAIN && menuItem == 2)
      menuNow = MENU_SHUTTER;
  
  }

  
}
//**********************************************
void canvas_shutter(){

}
//**********************************************
//Loop
void loop() {
  stick.loop();
  
  if( canvas == CANVAS_POSITION ){
    canvas_position();
  } else if ( canvas == CANVAS_MENU ){
    
    if( stick.getOK() ){
      canvas_settings( BUTTON_OK );
    }else canvas_settings( stick.getArrow() );
    
  }else if (canvas == CANVAS_SHUTTER ){
     canvas_shutter();
  }
}
