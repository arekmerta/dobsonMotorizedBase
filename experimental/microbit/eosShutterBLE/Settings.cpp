/*
 * Helper class to store settings
 */

//Settings
#define SETTING_ADAPTIVE_LIGHT      0b00000001
#define SETTING_SPEED_SECONDS_SPLIT 0b00000010
#define SETTING_USE_BULB_MODE       0b00000100

class Settings{
  private:
    bool _adaptiveLight     = false;
    bool _speedSecondsSplit = false;
    bool _useBulbMode       = false;
 
  public:
    void set(int val){
      _adaptiveLight     = val & SETTING_ADAPTIVE_LIGHT?true:false;
      _speedSecondsSplit = val & SETTING_SPEED_SECONDS_SPLIT?true:false;
      _useBulbMode       = val & SETTING_USE_BULB_MODE?true:false;
    }
    bool getAdaptiveLight(){
      return _adaptiveLight;
    }
    
    bool getSpeedSecondsSplit(){
      return _speedSecondsSplit;
    }

    bool getUseBulbMode(){
      return _useBulbMode;
    }
};
