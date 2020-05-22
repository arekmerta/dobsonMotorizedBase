/*
 * uczymy.edu.pl - based on BLEPeripheral example
 */
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
// minor byte: no of shots; major byte: speed in seconds
BLEShortCharacteristic    shooterConfigCharacteristic   = BLEShortCharacteristic ("2f93708401014730ae11df4a514bdfb3", BLERead | BLEWrite);
//minor byte: current shot; major byte: status, as: 0: idle (not started); 1: busy: shooting now; 2: about to start;  3: finished
#define STATUS_IDLE 0x0000
#define STATUS_BUSY 0x0100
#define STATUS_STARTING 0x0200
#define STATUS_FINISHED 0x0300

BLEShortCharacteristic    shooterProgressCharacteristic = BLEShortCharacteristic ("2f93708401024730ae11df4a514bdfb3", BLERead | BLENotify);

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
      //BLE
      bleUpdateProgress( 0, STATUS_STARTING );
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
        //Inform on started shot
        bleUpdateProgress(_nShotNow, STATUS_BUSY);
      }
    }else{
      //Inform on shooting end
      bleUpdateProgress( _nShots, STATUS_FINISHED );
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
      //Make sure the pixel is dimmed
      int col = _nShotNow % 5;
      int row = ( _nShotNow % 20 )  / 5 ;
      microbit.drawPixel( col, row, LED_OFF);
      
      _nShotNow+=1;
      inShot = false;
      //Wait timer to start next shot
      myTimer = millis();
      inWait = true;
      
    }
    return true;
  }
};

/*
 * Shutter class instance
 * No of shots, time, timeout between shots
 */
CameraShutter cs(5, 10, 2);

/*
 * Update BLE notifications
 */
void bleUpdateProgress(int nShotNow, int nStatus){
  shooterProgressCharacteristic.setValue( nShotNow + nStatus );
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
    int val = shooterConfigCharacteristic.value();

    int nShots   = val & 0x00FF;
    int nTimeOut = (val & 0xFF00 ) >> 8;

    Serial.print("BLE: got shots: ");
    Serial.println(nShots);

    Serial.print("BLE: got timeout: ");
    Serial.println(nTimeOut);

    cs.setShots( nShots );
    cs.setTimeout( nTimeOut * 1000 );
    
    Serial.println("Starting shooting session...");
    started = true;
  }
 
  started = cs.loop( started );
  
  if (! digitalRead(PIN_BUTTON_A)) {
    started = true;
  }
}
