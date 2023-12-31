///////////////////////////////////////////////////////////////////////////////////////////////////
//// FILE -> Utilities 
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
//// SECTION -> Definitions
///////////////////////////////////////////////////////////////////////////////////////////////////

#define GLOBALERRORS__MAX_ERRORS 6             // Maximum number of errors that can be stored at one time.
#define GLOBALERRORS__ASSERT_RESET 1           // 1 = Will restart on valid assert/deny, 0 = wont restart.

#define BOARDCONTROLLER__ENABLE_WDT 1             // 1 = enabled, 0 = not enabled
#define BOARDCONTROLLER__WDT_DURATION_MAX 300000 // Roughly 5 minutes
#define BOARDCONTROLLER__WDT_DURATION_VAL 5      // Warning offset reg value. 
#define BOARDCONTROLLER__WDT_DURATION_CYCLES 250 

#define BOARDCONTROLLER__IT_DURATION_MIN 30

#define BOARDCONTROLLER__SLEEP_CLOCK_HZ 48000000

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Program Settings
///////////////////////////////////////////////////////////////////////////////////////////////////

struct Setting {
  SETTING_ID id;
  int16_t value;  
};

// #Singleton
class GlobalSettings_ {
  private:
    static int16_t currentIndex;
    static Setting *settingArray;
    static int16_t settingArraySize;

  public:
    static void init(Setting *settings, int16_t size);

    static const int16_t &getSetting(const SETTING_ID settingID);

    static const bool setSetting(const SETTING_ID settingID, const int16_t newValue);

    const int16_t &operator [] (const SETTING_ID settingID);

  private:
    GlobalSettings_() {}

    static Setting *findSetting(const SETTING_ID targID);
}; 
extern GlobalSettings_ &SettingSys; 


///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Error Utility
///////////////////////////////////////////////////////////////////////////////////////////////////

// Method macros -> USE THESE
#define THROW(typeID) ErrorSys.report(typeID, __FUNCTION__, __FILE__, __LINE__)
#define ASSERT(statement, typeID) ErrorSys.assert(statement, typeID, __FUNCTION__, __FILE__, __LINE__)
#define DENY(statement, typeID) ErrorSys.deny(statement, typeID, __FUNCTION__, __FILE__, __LINE__)

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

    static ERROR_ID getLastAssert(); // NOT WORKING -> DEPENDS ON RESTART ERRORS WORKING
    
    static void clearLastAssert(); // NOT WORKING -> DEPENDS ON RESTART ERRORS WORKING

  private:
    GlobalErrors_() {}

    static void printError(ERROR_ID errorType, const char *funcName, const char *fileName, int16_t lineNum);
};
extern GlobalErrors_ &ErrorSys;


///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> System Controller
///////////////////////////////////////////////////////////////////////////////////////////////////

// #Singleton
class SystemController_ {
  private:
    static bool wdtInitialized;
    static bool itInitialized;
    static bool sleepInitialized;
 
  public:
    static void forceRestart();

    static void setRestartFlag(RESTART_FLAG_ID flag); // NOT WORKING

    static RESTART_FLAG_ID getRestartFlag(); // NOT WORKING

    static void clearRestartFlag(); // NOT WORKING

    static void sleep(uint8_t sleepMode, int32_t maxDuration = 0, void (*wakeSubscriber)(bool onTimeout) = nullptr); 

    static int32_t beginWatchdogTimer(int32_t duration, voidFuncPtr watchdogSubscriber = nullptr); 

    static void restartWatchdogTimer(); 

    static void disableWatchdogTimer();

    static void beginInterruptTimer(int32_t duration, voidFuncPtr subscriber);

    static void restartInterruptTimer();

    static void disableInterruptTimer();

    static int32_t getFreeRam();

  private:
    SystemController_() {}

    static void initSleep();

    static void initWDT(); 

    static void startWDT(uint8_t timeoutVal, uint8_t offsetVal, bool enableWM, 
      uint8_t closedIntervalVal = 0);
};
extern SystemController_ &System;

