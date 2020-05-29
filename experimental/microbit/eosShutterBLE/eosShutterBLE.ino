/*
 * uczymy.edu.pl - based on BLEPeripheral example
 */
#include <Arduino.h>
#include <BLEPeripheral.h>

#include "BLEUnsignedShortArrayCharacteristic.h"
#include "LightSensor.cpp"
#include "Settings.cpp"
#include "ScreenDisplay.h"

#define LIGHT_ADJUSTMENT_FACTOR 5
#define LIGHT_SPEED_CHANGE_THRESHOLD 5
#define MICROBIT_SHUTTER_PIN 2

//calculate
#define diffProc(a,b) ((int)(abs((float)(a-b)/(float)b)*100.))*(a>b?-1:1)


// create peripheral instance, see pinouts above percentage diff between two values
BLEPeripheral            blePeripheral        = BLEPeripheral();

// create service
BLEService               cameraShutterService          = BLEService            ("2f93708401004730ae11df4a514bdfb3");

// create switch and button characteristic; based on array of ushort:
// - 1st: no of shots; 
// - 2nd: speed in seconds
// - 3rd: byte - elapse in seconds
// - 4th byte: the settings (see Settings class)
BLEUnsignedShortArrayCharacteristic    shooterConfigCharacteristic   = BLEUnsignedShortArrayCharacteristic ("2f93708401014730ae11df4a514bdfb3", BLEWrite, 4);

//Status and progress method:
#define STATUS_IDLE 0x00
#define STATUS_BUSY 0x01
#define STATUS_STARTING 0x02
#define STATUS_FINISHED 0x03
#define STATUS_ELAPSE 0x04
#define STATUS_ABORTED 0x13
#define STATUS_SPEED_ADJUST 0x21

BLEUnsignedShortArrayCharacteristic    shooterProgressCharacteristic = BLEUnsignedShortArrayCharacteristic ("2f93708401024730ae11df4a514bdfb3", BLERead | BLENotify, 2);

/*
 * Update BLE notifications
 */
void bleUpdateProgress(unsigned short nShotNow, unsigned short nStatus){
  unsigned short arr[2] = {nShotNow, nStatus };
  shooterProgressCharacteristic.setUShortValues( arr );
  blePeripheral.poll();
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

  //Adaptive light
  bool _adaptiveLight = true;
  LightSensor _ls;

  //Screen display
  ScreenDisplay *_sd = NULL;

    
  int triggerShot(){
    Serial.print("Shooting #");
    Serial.println(getShotNow());
    analogWrite(MICROBIT_SHUTTER_PIN, 0);
  }
  
  int stopShot(){
    pinMode(MICROBIT_SHUTTER_PIN, INPUT);
    Serial.println("\t...End");
  }

  void adjustLight(boolean dryRun){
    static int _firstSpeed = getSpeedMS();
    static int _lastMeasure = 100;
    //Clear display for measures
    _sd->clear();

    //Make a few measures to "heat" the sensor
    if( dryRun ){
      for( int i = 0; i < 5; i++){
           _lastMeasure = _ls.measure( );
           delay(20);
      }
      //Restore display
      _sd->restore(); 
       Serial.print("Got initial light measure: ");
      Serial.println(_lastMeasure);
      return;
    }
    
    int measure = _ls.measure( );
    int diff = diffProc(measure, _lastMeasure);
    
    Serial.print("Got light measure: ");
    Serial.print(measure);
    Serial.print(", previous: ");
    Serial.print( _lastMeasure );
    Serial.print(", diff: ");
    Serial.println( diff );
    
    if( abs(diff) > LIGHT_SPEED_CHANGE_THRESHOLD ){
        //Now, modify the time by doubling the percentage
        int32_t newSpeed = getSpeedMS() + (_firstSpeed * ( -1 * diff * LIGHT_ADJUSTMENT_FACTOR ))/100;
      
        Serial.print( "Speed change from: " );
        Serial.print( getSpeedMS() );
        Serial.print( " to " );
        Serial.println( newSpeed );
        
        setSpeedMS(
            newSpeed
          );
        bleUpdateProgress( newSpeed, STATUS_SPEED_ADJUST );
        _lastMeasure = measure;
      }
      _sd->restore(); 
    }
    
  
public:
  /*
   * nShots: number of shots
   * nSpeedMS: speed of shutter, in miliseconds
   * nElapseMS: time between shots, in miliseconds
   * lightAdjust: if true - adapt shutter time to the changing light
   * sd: screen display
   */
  CameraShutter(
     unsigned int nShots, 
     int32_t nSpeedMS, 
     int32_t nElapseMS, 
     bool lightAdjust,
     ScreenDisplay *sd){
      
    pinMode(0, INPUT);
    _nShots = nShots;
    _nSpeedMS = nSpeedMS;
    _nElapseMS = nElapseMS;
    _adaptiveLight = lightAdjust;
    _sd = sd;
    _sd->fillScreen(LED_OFF);

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

  int32_t getSpeedMS(){
    return _nSpeedMS;
  }

  boolean setElapseMS(int32_t nElapseMS){
    _nElapseMS = nElapseMS;
  }

  boolean setAdaptiveLight(bool val){
    _adaptiveLight = val;
    Serial.print("Adaptive light set to: ");
    Serial.println(val);
  }

  boolean loop(boolean started){
    static bool _lastStarted = started;
    static bool inShot = false;
    static bool inWait = false;
    static int32_t myTimer = millis();
    static int32_t blinkTimer = millis();
    static bool pixel_on_off = false;
  
    //Re-init
    if( _lastStarted != started ){
      Serial.println("Resetting...");
      stopShot();
      _sd->fillScreen(LED_OFF);
      myTimer = millis();
      _nShotNow = 0;
      inShot = false;     
      inWait = false;
      _lastStarted = started;
      //BLE
      if( started ){
        bleUpdateProgress( 0, STATUS_STARTING );
        if( _adaptiveLight ){
          //initial measure
          adjustLight( true );
        }
      }else{
        bleUpdateProgress( 0, STATUS_FINISHED );
      }
    }

    if( !started ){
      _sd->fillScreen(LED_ON);
      delay(200);
      _sd->fillScreen(LED_OFF);
      delay(200);
      _lastStarted = started;
      return false;
    }
  
    if( inWait ){
      //that was last shot, do not wait, finish
      if ( _nShotNow >= _nShots ){
        return false;
      }else
      if( (millis() - myTimer) > _nElapseMS ){
        inWait = false;
        _sd->drawPixel( 4, 4, LED_OFF);
        
        if( _adaptiveLight ){
          adjustLight( false );
        }
            
      }else{
        _sd->drawPixel( 4, 4, LED_ON);
        return true;
      }
    }
    
    if( inShot == false ){
      int item=( _nShotNow / 20 ) * 20;
      for(int row = 0; row < 4; row++)
        for(int col = 0; col < 5; col ++)
    
        {
          if( item < _nShots ){
            _sd->drawPixel( col, row, ( item < _nShotNow ? LED_OFF:LED_ON ) );
          }else
            _sd->drawPixel( col, row, LED_OFF);
          item++;
        }
    
      int pagesLeft = _nShots / 20 - _nShotNow / 20 ;
      for(int i = 0; i < 4; i++){
        _sd->drawPixel(i, 4, (i <= pagesLeft ? LED_ON:LED_OFF));
      }
    }
  
    if( _nShotNow < _nShots ){
      if( inShot == false ){
        inShot = true;
        triggerShot();
        myTimer = millis();     
        blinkTimer = millis();
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
      _sd->drawPixel( col, row, LED_OFF);  
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

      if( ( millis() - blinkTimer ) > 100 ){
        _sd->drawPixel( col, row, pixel_on_off==false?LED_ON:LED_OFF);
        //delay(100);
        pixel_on_off = !pixel_on_off;
        blinkTimer = millis();
      }
    }
    return true;
  }
};

/*
 * Instances of used classes
 */
Settings settings;

ScreenDisplay sd;

CameraShutter cs(11, 2000, 5000, true, &sd);

/*
 * Setup
 */
void setup() {
  Serial.begin(9600);

  settings.set(SETTING_ADAPTIVE_LIGHT);
  
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

}

/*
 * Loop function
 */
void loop() {
  static boolean started = false;

  // poll peripheral
  blePeripheral.poll();
  // poll display
  sd.loop();
  
  if (shooterConfigCharacteristic.written() ) {
    
    unsigned short vals[3];
    shooterConfigCharacteristic.getUShortValues(vals);

    int nShots  = vals[0];
    int nSpeed  =  vals[1];
    int nElapse = vals[2];
    int nSettings = vals[3];
    
    Serial.print("BLE: got shots: ");
    Serial.println(nShots);

    Serial.print("BLE: got speed: ");
    Serial.println(nSpeed);

    Serial.print("BLE: got elapse: ");
    Serial.println(nElapse);

    Serial.print("BLE: got settings: ");
    Serial.println(nSettings, HEX);

    settings.set( nSettings );

    cs.setAdaptiveLight ( settings.getAdaptiveLight() );

    if( settings.getSpeedSecondsSplit() ){
      Serial.println("BLE: speed: applying 1/x");

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

  // poll shutter loop
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
