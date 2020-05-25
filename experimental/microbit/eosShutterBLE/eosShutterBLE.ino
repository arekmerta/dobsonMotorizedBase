/*
 * uczymy.edu.pl - based on BLEPeripheral example
 */
#include <Adafruit_Microbit.h>
#include <BLEPeripheral.h>

#include "BLEUnsignedShortArrayCharacteristic.h"

Adafruit_Microbit_Matrix microbit;

// define pins (varies per shield/board)
#define BLE_REQ     10
#define BLE_RDY     2
#define BLE_RST     9

#define MICROBIT_SHUTTER_PIN 2

// create peripheral instance, see pinouts above
BLEPeripheral            blePeripheral        = BLEPeripheral();

// create service
BLEService               cameraShutterService          = BLEService            ("2f93708401004730ae11df4a514bdfb3");

// create switch and button characteristic
// 1st ushort: no of shots; 2nd ushort: speed in seconds, 3rd byte - elapse in seconds
//for the 2nd ushort: MSB if set to 1, rest keeps parts of seconds (1/)
BLEUnsignedShortArrayCharacteristic    shooterConfigCharacteristic   = BLEUnsignedShortArrayCharacteristic ("2f93708401014730ae11df4a514bdfb3", BLEWrite, 3);
//minor byte: current shot; major byte: status, as: 0: idle (not started); 1: busy: shooting now; 2: about to start;  3: finished
#define STATUS_IDLE 0x00
#define STATUS_BUSY 0x01
#define STATUS_STARTING 0x02
#define STATUS_FINISHED 0x03
#define STATUS_ELAPSE 0x04

#define STATUS_ABORTED 0x13

BLEUnsignedShortArrayCharacteristic    shooterProgressCharacteristic = BLEUnsignedShortArrayCharacteristic ("2f93708401024730ae11df4a514bdfb3", BLERead | BLEWrite | BLENotify, 2);

//Pin #0 - large pad - analog in
//Pin #1 - large pad - analog in
//Pin #2 - large pad - analog in
 
/*
 * Setup
 */
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
  blePeripheral.addAttribute(shooterProgressCharacteristic);
  
  blePeripheral.begin();

  //BLE
  bleUpdateProgress(0, STATUS_IDLE );

  microbit.begin();
  microbit.fillScreen(LED_OFF);
}

/*
 * Camera Shutter class
 */
class CameraShutter{
private:
  int _nShots;
  int _nShotNow = 0;
  int32_t _nSpeedMS = 0;
  int32_t _nElapseMS = 0;
  
  int triggerShot(){
    Serial.print("Shooting #");
    Serial.println(getShotNow());
    analogWrite(MICROBIT_SHUTTER_PIN, 0);
  }
  int stopShot(){
    pinMode(MICROBIT_SHUTTER_PIN, INPUT);
    Serial.println("\t...End");
  }
  
public:
  /*
   * nShots: number of shots
   * nSpeedMS: speed of shutter, in miliseconds
   * nElapseMS: time between shots, in miliseconds
   */
  CameraShutter(unsigned int nShots, int32_t nSpeedMS, int32_t nElapseMS){
    pinMode(0, INPUT);
    _nShots = nShots;
    _nSpeedMS = nSpeedMS;
    _nElapseMS = nElapseMS;
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

  boolean setSpeedMS(int nSpeedMS){
    _nSpeedMS = nSpeedMS;
  }

  boolean setElapseMS(int nElapseMS){
    _nElapseMS = nElapseMS;
  }

  boolean loop(boolean started){
    static bool _lastStarted = started;
    static bool inShot = false;
    static bool inWait = false;
    static int32_t myTimer = millis();
    static bool pixel_on_off = false;
  
    //Re-init
    if( _lastStarted != started ){
      Serial.println("Resetting...");
      stopShot();
      microbit.fillScreen(LED_OFF);
      myTimer = millis();
      _nShotNow = 0;
      inShot = false;     
      inWait = false;
      _lastStarted = started;
      //BLE
      if( started ){
        bleUpdateProgress( 0, STATUS_STARTING );
      }else{
        bleUpdateProgress( 0, STATUS_FINISHED );
      }
    }

    if( !started ){
      microbit.fillScreen(LED_ON);
      delay(200);
      microbit.fillScreen(LED_OFF);
      delay(200);
      _lastStarted = started;
      return false;
    }
    
    if( inWait ){
      if( (millis() - myTimer) > _nElapseMS ){
        inWait = false;
        microbit.drawPixel( 4, 4, LED_OFF);
        
      }else{
        microbit.drawPixel( 4, 4, LED_ON);
        return true;
      }
    
    }
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
        //Inform on started shot
        bleUpdateProgress(_nShotNow, STATUS_BUSY);
      }
    }else{
      return false;
    }

    int32_t timeDiff = millis() - myTimer ;
  
    if( inShot && ( timeDiff > _nSpeedMS ) ){
      stopShot();
      Serial.print("Recorded time:");
      Serial.println(timeDiff);
      //Make sure the pixel is dimmed
      int col = _nShotNow % 5;
      int row = ( _nShotNow % 20 )  / 5 ;
      microbit.drawPixel( col, row, LED_OFF);  
      _nShotNow+=1;
      inShot = false;
      //Wait timer to start next shot
      inWait = true;   
      //BLE
      bleUpdateProgress(_nShotNow, STATUS_ELAPSE);
      myTimer = millis();
    }

    //There may be no time to flip leds
    if( inShot && ( timeDiff > 100 ) ){
      int col = _nShotNow % 5;
      int row = ( _nShotNow % 20 )  / 5 ;

      microbit.drawPixel( col, row, pixel_on_off==false?LED_ON:LED_OFF);
      delay(100);
      pixel_on_off = !pixel_on_off;
    }
    return true;
  }
};

/*
 * Shutter class instance
 * No of shots, speed, elapse
 */
CameraShutter cs(5, 2000, 5000);

/*
 * Update BLE notifications
 */
void bleUpdateProgress(unsigned short nShotNow, unsigned short nStatus){
  unsigned short arr[2] = {nShotNow, nStatus };
  shooterProgressCharacteristic.setUShortValues( arr );
  blePeripheral.poll();
}

/*
 * Loop function
 */
void loop() {
  static boolean started = false;

  // poll peripheral
  blePeripheral.poll();

  if (shooterConfigCharacteristic.written() ) {
    /*
    unsigned int val = shooterConfigCharacteristic.value();

    int nShots   = val & 0x00FF;
    int nSpeed = (val & 0xFF00 ) >> 8;
    int nElapse = ( (val & 0xFF0000 ) >> 16 )*1000;
    */
    unsigned short vals[3];
    shooterConfigCharacteristic.getUShortValues(vals);

    int nShots   = vals[0];
    int nSpeed =  vals[1];
    int nElapse = vals[2];
    
    Serial.print("BLE: got shots: ");
    Serial.println(nShots);

    Serial.print("BLE: got speed: ");
    Serial.println(nSpeed);

    Serial.print("BLE: got elapse: ");
    Serial.println(nElapse);


    if( nSpeed & 0x8000 ){
      //parts of second send
      nSpeed = nSpeed & 0b01111111;
      nSpeed = 1000 / nSpeed;
    }else{
      nSpeed = nSpeed * 1000;
    }

    nElapse = nElapse * 1000;

    Serial.print("BLE: got [ms] speed: ");
    Serial.println(nSpeed);

    Serial.print("BLE: got [ms] elapse: ");
    Serial.println(nElapse);

    cs.setShots( nShots );
    cs.setSpeedMS( nSpeed );
    cs.setElapseMS( nElapse );
    
    Serial.println("Starting shooting session...");
    started = true;
  }
 
  started = cs.loop( started );
  
  if (! digitalRead(PIN_BUTTON_A)) {
    started = true;
  }
  if (! digitalRead(PIN_BUTTON_B)) {
    started = false;
    //BLE
     bleUpdateProgress( 0, STATUS_ABORTED );
  }
}
