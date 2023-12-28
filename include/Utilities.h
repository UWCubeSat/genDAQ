///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Pre-Processor & General Declarations
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <Arduino.h>
#include <GlobalDefs.h>

struct Setting;
class GlobalSettings_;

struct Error;
class GlobalErrors_;

class BoardController_;
void WDT_Handler(void); 

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Program Settings
///////////////////////////////////////////////////////////////////////////////////////////////////

// #Singleton
class GlobalSettings_ {
  private:
    static Setting settingArray[GLOBALSETTINGS__MAX_SETTINGS];
    static uint8_t currentIndex;

  public:
    const int16_t &declareSetting(const SETTING_ID settingID, const int16_t initialValue);

    const int16_t &getSetting(const SETTING_ID settingID);

    const bool setSetting(const SETTING_ID settingID, const int16_t newValue);

    ///////// TO DO -> ADD SYNC SETTING FUNCTION
    ///////// TO DO -> SAVE SETTING? FOR SAVING BETWEEN RELOADS?

    const int16_t &operator [] (const SETTING_ID settingID);

  private:
    GlobalSettings_() {}

    Setting *findSetting(const SETTING_ID targID);
}; 
extern GlobalSettings_ &SettingSys; 


///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Error Utility
///////////////////////////////////////////////////////////////////////////////////////////////////

// This value is saved between program restarts
uint8_t previousAssert __attribute__ ((section (".no_init"))); 

#define ASSERT(statement, errorType) Errors.assert()

// #Singleton
class GlobalErrors_ {
  private:
    static uint8_t errorArray[GLOBALERRORS__MAX_ERRORS];
    static uint8_t currentIndex;
    static bool restartActive;
  
  public:
    void report(ERROR_ID errorType, const char *funcName, const char *fileName, int16_t lineNum);

    bool assert(bool statement, ERROR_ID errorType, const char *funcName, const char *fileName, int16_t lineNum);

    bool deny(bool statement, ERROR_ID errorType, const char *funcName, const char *fileName, int16_t lineNum);

    ERROR_ID getLastError() const;
    
    uint8_t getErrorCount() const;

    ERROR_ID getLastAssert() const;
    
    void clearLastAssert() const;

  private:
    GlobalErrors_() {}

    void printError(ERROR_ID errorType, const char *funcName, const char *fileName, int16_t lineNum);
};
extern GlobalErrors_ &ErrorSys;


///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Board Controller
///////////////////////////////////////////////////////////////////////////////////////////////////

// This value is saved between program restarts
uint8_t restartFlag __attribute__ ((section (".no_init")));

// For custom WDT warning handler.
typedef void (*wdtHandler)(void);

// #Singleton
class BoardController_ {
  private:
    static bool wdtInitialized;
 
  public:
    void forceRestart(); // DONE

    void setRestartFlag(RESTART_FLAG_ID flag); // DONE

    RESTART_FLAG_ID getRestartFlag(); // DONE

    void clearRestartFlag(); // DONE

    int32_t beginWatchdogTimer(int32_t duration, wdtHandler watchdogSubscriber = nullptr, bool restartOnExpire = true); // DONE

    void restartWatchdogTimer(); // DONE

    void disableWatchdogTimer(); // DONE

    int32_t sleep(int32_t duration, wdtHandler timeoutSubscriber = nullptr);

  private:
    BoardController_() {}

    void initWDT();

    void startWDT(uint8_t timeoutVal, uint8_t offsetVal, bool enableWM, // DONE -> EXCEPT SLEEP
      uint8_t closedIntervalVal = 0);

    void processDurationWDT(int32_t &targDuration, uint8_t &offsetValReturn, //DONE
      int32_t &estimateDurationReturn);
};
extern BoardController_ &Board;

