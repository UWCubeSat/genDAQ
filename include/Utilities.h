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

#define GLOBALSETTINGS__MAX_SETTINGS 16 // Maximum number of settings.

#define GLOBALERRORS__MAX_ERRORS 6             // Maximum number of errors that can be stored at one time.
#define GLOBALERRORS__ASSERT_RESET 1           // 1 = Will restart on valid assert/deny, 0 = wont restart.
#define GLOBALERRORS__ASSERT_RESTART_DELAY 500 // Restart delay in valid asser/deny (milliseconds)

#define BOARDCONTROLLER__ENABLE_WDT 1             // 1 = enabled, 0 = not enabled
#define BOARDCONTROLLER__WDT_DURATION_MAX 300000 // Roughly 5 minutes
#define BOARDCONTROLLER__WDT_DURATION_VAL 5      // Warning offset reg value. 
#define BOARDCONTROLLER__WDT_DURATION_CYCLES 250 

#define BOARDCONTROLLER__IT_DURATION_MIN 30

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Program Settings
///////////////////////////////////////////////////////////////////////////////////////////////////

// #Singleton
class GlobalSettings_ {
  private:
    static Setting settingArray[GLOBALSETTINGS__MAX_SETTINGS];
    static uint8_t currentIndex;

  public:
    static const int16_t &declareSetting(const SETTING_ID settingID, const int16_t initialValue);

    static const int16_t &getSetting(const SETTING_ID settingID);

    static const bool setSetting(const SETTING_ID settingID, const int16_t newValue);

    ///////// TO DO -> ADD SYNC SETTING FUNCTION
    ///////// TO DO -> SAVE SETTING? FOR SAVING BETWEEN RELOADS?

    const int16_t &operator [] (const SETTING_ID settingID);

  private:
    GlobalSettings_() {}

    static Setting *findSetting(const SETTING_ID targID);
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
    static void report(ERROR_ID errorType, const char *funcName, const char *fileName, int16_t lineNum);

    static bool assert(bool statement, ERROR_ID errorType, const char *funcName, const char *fileName, int16_t lineNum);

    static bool deny(bool statement, ERROR_ID errorType, const char *funcName, const char *fileName, int16_t lineNum);

    static ERROR_ID getLastError();
    
    static uint8_t getErrorCount();

    static ERROR_ID getLastAssert();
    
    static void clearLastAssert();

  private:
    GlobalErrors_() {}

    static void printError(ERROR_ID errorType, const char *funcName, const char *fileName, int16_t lineNum);
};
extern GlobalErrors_ &ErrorSys;


///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Board Controller
///////////////////////////////////////////////////////////////////////////////////////////////////

// This value is saved between program restarts
uint8_t restartFlag __attribute__ ((section (".no_init")));

// For custom WDT warning handler.
typedef void (*interruptFunc)(void);

// #Singleton
class BoardController_ {
  private:
    static bool wdtInitialized;
    static bool itInitialized;
    static bool sleepInitialized;
 
  public:
    static void forceRestart();

    static void setRestartFlag(RESTART_FLAG_ID flag);

    static RESTART_FLAG_ID getRestartFlag(); 

    static void clearRestartFlag();

    static int32_t beginWatchdogTimer(int32_t duration, interruptFunc watchdogSubscriber = nullptr); 

    static void restartWatchdogTimer(); 

    static void disableWatchdogTimer();

    static void beginInterruptTimer(int32_t duration, interruptFunc subscriber);

    static void restartInterruptTimer();

    static void disableInterruptTimer();

    static void sleep(uint8_t sleepMode, int32_t maxDuration = 0, void (*wakeSubscriber)(bool timeoutExpire) = nullptr); 

    static int32_t getFreeRam();

  private:
    BoardController_() {}

    static void initWDT(); 

    static void startWDT(uint8_t timeoutVal, uint8_t offsetVal, bool enableWM, 
      uint8_t closedIntervalVal = 0);

    static void initSleep();

    
};
extern BoardController_ &Board;

