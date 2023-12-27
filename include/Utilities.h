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

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Program Settings
///////////////////////////////////////////////////////////////////////////////////////////////////

// #Singleton
class GlobalSettings_ {
  private:
    static Setting settingArray[SETTINGS_AMOUNT_MAX];
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
    static uint8_t errorArray[ERRORS_AMOUNT_MAX];
    static uint8_t currentIndex;
    static bool restartActive;
  
  public:
    void report(ERROR_TYPE errorType, const char *funcName, const char *fileName, int16_t lineNum);

    bool assert(bool statement, ERROR_TYPE errorType, const char *funcName, const char *fileName, int16_t lineNum);

    bool deny(bool statement, ERROR_TYPE errorType, const char *funcName, const char *fileName, int16_t lineNum);

    ERROR_TYPE getLastError() const;
    
    uint8_t getErrorCount() const;

    ERROR_TYPE getLastAssert() const;
    
    void clearLastAssert() const;

  private:
    GlobalErrors_() {}

    void printError(ERROR_TYPE errorType, const char *funcName, const char *fileName, int16_t lineNum);
};
extern GlobalErrors_ &ErrorSys;


///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Board Controller
///////////////////////////////////////////////////////////////////////////////////////////////////

// This value is saved between program restarts
uint8_t restartMsg __attribute__ ((section (".no_init"))); 

// #Singleton
class BoardController_ {
  public:
    // @brief: Immediatly initiates program restart.
    // @param: reason -> Reason for the restart.
    // @NOTE: The provided restart msg (reason) is saved through the
    //        restart and can be read afterwards using getRestartMsg();
    void restartProgram(BOARD_RESTART_MSG reason);

    // @param: durration -> Duration of sleep (seconds).
    void sleep(int16_t durration);

    BOARD_RESTART_MSG getRestartMsg();

  private:
    BoardController_() {}

};
extern BoardController_ &Board;