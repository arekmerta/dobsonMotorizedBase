/*
 * Helper class to store settings
 */
 
#define SETTING_ADAPTIVE_LIGHT 0b00000001

class Settings{
  private:
    bool _adaptiveLight = false;
 
  public:
    void set(unsigned char val){
      if(val & SETTING_ADAPTIVE_LIGHT ){
        _adaptiveLight = true;
      }
    }
  
    bool getAdaptiveLight(){
      return _adaptiveLight;
    }
    
};

