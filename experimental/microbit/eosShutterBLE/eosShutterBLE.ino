#include <Adafruit_Microbit.h>
#include <BLEPeripheral.h>

Adafruit_Microbit_Matrix microbit;

// define pins (varies per shield/board)
#define BLE_REQ     10
#define BLE_RDY     2
#define BLE_RST     9

// create peripheral instance, see pinouts above
BLEPeripheral            blePeripheral        = BLEPeripheral(BLE_REQ, BLE_RDY, BLE_RST);

// create service
BLEService               cameraShutterService          = BLEService            ("2f93708401004730ae11df4a514bdfb3");
// create switch and button characteristic
//1st byte: no of shots; 2nd byte: delay in seconds
BLEIntCharacteristic    shooterConfigCharacteristic   = BLEIntCharacteristic ("2f93708401014730ae11df4a514bdfb3", BLERead | BLEWrite);
//0: stop; 1: start
BLECharCharacteristic    shooterStartCharacteristic    = BLECharCharacteristic ("2f93708401024730ae11df4a514bdfb3", BLERead | BLEWrite);
//1st byte: current shot; 2nd byte: no of shots
BLEIntCharacteristic    shooterProgressCharacteristic = BLEIntCharacteristic ("2f93708401034730ae11df4a514bdfb3", BLERead | BLENotify);

//Pin #0 - large pad - analog in
//Pin #1 - large pad - analog in
//Pin #2 - large pad - analog in
 

void setup() {
  Serial.begin(9600);
  
  pinMode(PIN_BUTTON_A, INPUT);
  pinMode(PIN_BUTTON_B, INPUT);

   Serial.println("Staring services...");
  //BLE part
  // set advertised local name and service UUID
  blePeripheral.setLocalName("Camera Shutter");
  blePeripheral.setAdvertisedServiceUuid(cameraShutterService.uuid());

  // add service and characteristics
  blePeripheral.addAttribute(cameraShutterService);
  blePeripheral.addAttribute(shooterConfigCharacteristic);
  blePeripheral.addAttribute(shooterStartCharacteristic);
  blePeripheral.addAttribute(shooterProgressCharacteristic);
  
  blePeripheral.begin();

 microbit.begin();
  microbit.fillScreen(LED_OFF);

}

class CameraShutter{
private:
  int _nShots;
  int _nShotNow = 0;
  int _nTimeOut = 0;
  int _nTimeOutBetweenShots = 0;
  
  int triggerShot(){
    Serial.print("Shooting #");
    Serial.println(getShotNow());
    analogWrite(0, 0);
  }
  int stopShot(){
    pinMode(0, INPUT);
    Serial.println("\t...End");
  }

  
public:
  CameraShutter(int nShots, int nTimeOutSeconds, int nTimeOutBetweenShots){
    pinMode(0, INPUT);
    _nShots = nShots;
    _nTimeOut = nTimeOutSeconds * 1000;
    _nTimeOutBetweenShots = nTimeOutBetweenShots * 1000;
  }

  int getShotNow(){
    return _nShotNow;
  }

  int getShots(){
    return _nShots;
  }

  boolean setShots(int nShots){
    _nShots = nShots;
  }

  boolean setTimeout(int nTimeOutSeconds){
    _nTimeOut = nTimeOutSeconds;
  }

  boolean loop(boolean started){
    static bool _lastStarted = started;
    static bool inShot = false;
    static bool inWait = false;
    static int32_t myTimer = millis();
  
    if( !started ){
      microbit.fillScreen(LED_ON);
      delay(200);
      microbit.fillScreen(LED_OFF);
      delay(200);
      _lastStarted = started;
      return false;
    }
    //Re-init
    if( _lastStarted != started ){
      microbit.fillScreen(LED_OFF);
      myTimer = millis();
      _nShotNow = 0;
      inShot = false;     
      inWait = false;
      _lastStarted = started;
    }

    if( inWait ){
      if( (millis() - myTimer) > _nTimeOutBetweenShots ){
        inWait = false;
        microbit.drawPixel( 4, 4, LED_OFF);
        
      }else{
        microbit.drawPixel( 4, 4, LED_ON);
        return true;
      }
    
    }
  
    //microbit.drawPixel( col, row, LED_ON);
    
    if( inShot == false ){
      int item=( _nShotNow / 20 ) * 20;
    
      for(int row = 0; row < 4; row++)
        for(int col = 0; col < 5; col ++)
    
        {
          if( item < _nShots ){
            microbit.drawPixel( col, row, ( item < _nShotNow ? LED_OFF:LED_ON ) );
          }else
            microbit.drawPixel( col, row, LED_OFF);
          item++;
        }
    
      int pagesLeft = _nShots / 20 - _nShotNow / 20 ;
      for(int i = 0; i < 5; i++){
        microbit.drawPixel(i, 4, (i <= pagesLeft ? LED_ON:LED_OFF));
      }
    }
  
    if( _nShotNow < _nShots ){
      if( inShot == false ){
        inShot = true;
        triggerShot();
        myTimer = millis();     
      }
    }else{
      return false;
    }
  
    if( inShot ){
      int col = _nShotNow % 5;
      int row = ( _nShotNow % 20 )  / 5 ;
      
      microbit.drawPixel( col, row, LED_OFF);
      delay(100);
      microbit.drawPixel( col, row, LED_ON);
      delay(100);
    }
    
    if( (millis() - myTimer ) > _nTimeOut ){
      stopShot();
      _nShotNow+=1;
      inShot = false;
      //Wait timer to start next shot
      myTimer = millis();
      inWait = true;
    }
    return true;
  }
};

//No of shots, time, timeout between shots
CameraShutter cs(5, 10, 2);

void loop() {
  static boolean started = false;

  // poll peripheral
  blePeripheral.poll();

  if (shooterConfigCharacteristic.written() ) {
    int val = shooterConfigCharacteristic.value();
    int nShots   = val >>16;
    int nTimeOut = val & 0x00FF;

    Serial.print("BLE: got shots: ");
    Serial.println(nShots);

    Serial.print("BLE: got timeout: ");
    Serial.println(nTimeOut);
    
  }/*
  if (shooterStartCharacteristic.written() ) {
    bool nStart   = (bool)shooterConfigCharacteristic.value();
    
    Serial.print("BLE: got start: ");
    Serial.println(nStart);
  }*/

  //started = cs.loop( started );
  if( started ){
    //end of sequence
  }
  if (! digitalRead(PIN_BUTTON_A)) {
    started = true;
  }
}
