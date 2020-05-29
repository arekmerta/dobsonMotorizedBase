/*
 * Helper class to store settings
 */

//Settings
#define SETTING_ADAPTIVE_LIGHT 0b00000001

class Settings{
  private:
    bool _adaptiveLight = false;
 
  public:
    void set(int val){
      if(val & SETTING_ADAPTIVE_LIGHT ){
        _adaptiveLight = true;
      }else
        _adaptiveLight = false;
    }
  
    bool getAdaptiveLight(){
      return _adaptiveLight;
    }
};
