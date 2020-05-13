//uczymy.edu.pl: based on multiple examples (list may not be complete):
//Based on BLE_Client example from https://github.com/espressif/arduino-esp32
//     author unknown
//     updated by chegewara
//
//Based on LCD library from: https://github.com/johnrickman/LiquidCrystal_I2C
//Based on ESP32 Hardware Serial2 Example from https://circuits4you.com/2018/12/31/esp32-hardware-serial2-example/
//Based on Adafruit example GPS_HardwareSerial from https://github.com/adafruit/Adafruit_GPS
//
//For more on my dobson stand go to: http://uczymy.edu.pl/wp/podstawa-teleskopu-celestron-c90-mak-typu-dobson/

#include <stdio.h> // for function sprintf
#include "BLEDevice.h"
#include <LiquidCrystal_I2C.h>
#include <Adafruit_GPS.h>

#include "joystick.cpp"
#include "lcd_menu.cpp"
#include "defs.h"

//Joystick
Joystick stick(BUTTON_GPIO, 20);

//GPS
#define RXD2 16
#define TXD2 17
Adafruit_GPS GPS(&Serial2);

//LCD

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

//Menus
#define MENU_MAIN          100
#define MENU_MAIN_POSITION 101
#define MENU_MAIN_GOTO     102
#define MENU_MAIN_CAMERA   103

#define MENU_GOTO          200
#define MENU_GOTO_1        201
#define MENU_GOTO_2        202


#define MENU_CAMERA        300
#define MENU_CAMERA_1      301
#define MENU_CAMERA_2      302
#define MENU_CAMERA_3      303


tMenuItem menus[]{
  MENU_MAIN,           MENU_TYPE_HEADING, MENU_NOSIBLING, "Main            ", MENU_NOSIBLING, 0, 0, 0,
  MENU_MAIN_POSITION,  MENU_TYPE_ACTION,  MENU_MAIN,      " Position       ", MENU_NOSIBLING, 0, 0, 0,
  MENU_MAIN_GOTO,      MENU_TYPE_SUBITEM, MENU_MAIN,      " GoTo           ", MENU_GOTO, 0, 0, 0,
  MENU_MAIN_CAMERA,    MENU_TYPE_SUBITEM, MENU_MAIN,      " Camera         ", MENU_CAMERA, 0, 0, 0,
  
  MENU_GOTO,           MENU_TYPE_HEADING, MENU_NOSIBLING, "GoTo            ", MENU_NOSIBLING, 0, 0, 0,
  MENU_GOTO_1,         MENU_TYPE_VALUE,  MENU_GOTO,      " --goto_1--     ", MENU_GOTO_1, 0, 10, 0,
  MENU_GOTO_2,         MENU_TYPE_VALUE,  MENU_GOTO,      " --goto_2--     ", MENU_GOTO_2, 0, 10, 0,

  MENU_CAMERA,         MENU_TYPE_HEADING, MENU_NOSIBLING, "Main            ", MENU_NOSIBLING, 0, 0, 0,
  MENU_CAMERA_1,       MENU_TYPE_ACTION,  MENU_CAMERA,    " Shutter(s)     ", MENU_CAMERA_1, 1, 30, 0,
  MENU_CAMERA_2,       MENU_TYPE_ACTION,  MENU_CAMERA,    " Shots(n)       ", MENU_CAMERA_2, 1, 50, 0,
  MENU_CAMERA_3,       MENU_TYPE_ACTION,  MENU_CAMERA,    " Dark(n)        ", MENU_CAMERA_2, 1, 20, 0,
 
//Always add last element - so that no need to add menu table size
  MENU_NOSIBLING,          MENU_TYPE_LAST,    MENU_NOSIBLING, "", MENU_NOSIBLING, 0, 0, 0,
  
};

LcdMenu lcdMenu(&lcd, menus, MENU_MAIN_POSITION);

//Canvas
#define CANVAS_POSITION 1
#define CANVAS_MENU     2
#define CANVAS_SHUTTER  3
static volatile int canvas = CANVAS_POSITION;
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

  //LCD
  log_iln("Starting LCD");
  lcd.init(); 
  lcd.backlight();
  lcd_print(0, 0, "Initializing...");
    
  log_iln("Starting Arduino BLE Client application...");
  //BLE
  BLEDevice::init("");
  pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());
  // Connect to the remove BLE Server.
  if( !(connected = connectToServer(pClient))){
    wasConnected = true;
  }else{
    lcd_print(0, 0, "BLE:connected   ");
    wasConnected = true;
  }
  //GPS
  log_iln("Starting GPS...");
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  GPS.begin(9600);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
} 
//**********************************************

void canvas_position(boolean firstEntry){
  int MAX_DIFF = 1;//%
  
  static float lastHeading = 0., lastYaw = 0.;
  static uint32_t gpsTimer = millis() - 10000;
  static uint32_t thingyDisconnectTimer = millis();

  //This is just to reset "refresh" settings
  if( firstEntry ){
    lastHeading = 0.;
    lastYaw = 0.;
    gpsTimer = millis() - 10000;
    thingyDisconnectTimer = millis();
  
  }

  //GPS
  while(GPS.available())GPS.read();
  
  if (GPS.newNMEAreceived()) {
    log_vln( GPS.lastNMEA() ); 
    if (!GPS.parse(GPS.lastNMEA())) return; 
  }
  
  if( !connected && wasConnected) {
    log_iln("---->DISCONNECTED");
    
    lcd_print(0, 0, "BLE:disconnected");
    
    lcd_print(0, 1, "                ");
    
    pClient->disconnect();
    thingyDisconnectTimer = millis();
    wasConnected = false;
  }
    
  if ( !connected && !wasConnected && ((millis() - thingyDisconnectTimer ) > SECONDS_30)){
    //connect
    connected = connectToServer(pClient);

    if( connected ){
      lcd_print( 0, 0, "BLE:connected   ");
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
  
  
  if (GPS.fix && (millis() - gpsTimer > SECONDS_5) ) {
    gpsTimer = millis();
    //log_i(GPS.latitude, 4); log_i(GPS.lat);
    //log_i(", ");
    //log_i(GPS.longitude, 4); log_iln(GPS.lon);
    //log_i("Altitude: "); log_iln(GPS.altitude);

    //Latitude
    lcd.setCursor(0, 2); 
    lcd.print( GPS.lat );
    lcd.print( " " );
    lcd.print( GPS.latitude );
    lcd.print("  ");
    //Date
    if(  GPS.day < 10 )lcd.print('0');
    lcd.print(GPS.day, DEC); lcd.print('/');
    if(  GPS.month < 10 )lcd.print('0');
    lcd.print(GPS.month, DEC); 

    //Long
    lcd.setCursor(0, 3); 
    lcd.print( GPS.lon );
    lcd.print( " " );
    lcd.print( GPS.longitude);
    lcd.print("  ");
    //Time
    if(  GPS.hour+2 < 10 )lcd.print('0');
    lcd.print(GPS.hour+2, DEC); lcd.print(':');
    if(  GPS.minute < 10 )lcd.print('0');
    lcd.print(GPS.minute, DEC); 
  }else if (!GPS.fix){
    lcd_print(0, 2, "GPS: no fix yet ");
    lcd_print(0, 3, "                ");
  }  
}
//**********************************************
void canvas_menu(char button){

  static int menuNow = MENU_NOSIBLING;

  if( menuNow == MENU_NOSIBLING ){
    lcdMenu.displayMenu( MENU_MAIN );
    menuNow = MENU_MAIN;
  }

  int res = lcdMenu.loop(button);

  if( res == MENU_MAIN_POSITION ){
    canvas = CANVAS_POSITION;
    menuNow = MENU_NOSIBLING;
    
  }
}
//**********************************************
void canvas_shutter(){

}
//**********************************************
//Loop
void loop() {
  static boolean firstEntry = true;
  
  stick.loop();
  
  if( canvas == CANVAS_POSITION ){
    if( stick.getOK() ){
      canvas = CANVAS_MENU;
    }else{
      canvas_position(firstEntry);
      firstEntry = false;
    }
  } 
  
  if ( canvas == CANVAS_MENU ){
    if( stick.getOK() ){  
      canvas_menu( BUTTON_OK );
    }else 
      canvas_menu( stick.getArrow() );
      //we are leaving menu
      if( canvas == CANVAS_POSITION ){
        firstEntry = true;  
      }
  }
}
