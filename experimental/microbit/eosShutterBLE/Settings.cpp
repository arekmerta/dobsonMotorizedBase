/*
 * Helper class to store settings
 */

//Settings
#define SETTING_ADAPTIVE_LIGHT      0b00000001
#define SETTING_SPEED_SECONDS_SPLIT 0b00000010

class Settings{
  private:
    bool _adaptiveLight     = false;
    bool _speedSecondsSplit = false;
 
  public:
    void set(int val){
      _adaptiveLight     = val & SETTING_ADAPTIVE_LIGHT?true:false;
      _speedSecondsSplit = val & SETTING_SPEED_SECONDS_SPLIT?true:false;
    }
    bool getAdaptiveLight(){
      return _adaptiveLight;
    }
    
    bool getSpeedSecondsSplit(){
      return _speedSecondsSplit;
    }
};
