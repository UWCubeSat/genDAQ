///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Pre-Processor & General Declarations
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <Utilities.h>

struct Setting;

#define GLOBALSETTINGS__MAX_SETTINGS 16 // Maximum number of settings.

#define GLOBALERRORS__MAX_ERRORS 6 // Maximum number of errors that can be stored at one time.
#define GLOBALERRORS__ASSERT_RESET 1 // 1 = Will restart on valid assert/deny, 0 = wont restart.
#define GLOBALERRORS__ASSERT_RESTART_DELAY 500 // Restart delay in valid asser/deny (milliseconds)

#define BOARDCONTROLLER__ENABLE_WDT 1 // 1 = enabled, 2 = not enabled
#define BOARDCONTROLLER__WDT_MIN_DURATION 32 // In microseconds
#define BOARDCONTROLLER__WDT_MAX_DURATION 3600000 // In microseconds

#define BOARDCONTROLLER__WDT_CLOCK_SPEED 1024 // Prescale of 32,767hz WDT clock is 1:32 -> 1024hz 
#define BOARDCONTROLLER__WDT_DISABLE_KEY 165 // Used for clearing watchdawg timer register.
#define BOARDCONTROLLER__WDT_LONG_DURATION_MIN 3000
#define BOARDCONTROLLER__WDT_LONG_DURATION_CYCLES 1000
#define BOARDCONTROLLER__WDT_LONG_DURATION_VAL 7 //Offset bit when duration > LONG DURATION MIN
#define BOARDCONTROLLER__WDT_DEFAULT_DURATION_CYCLES 250
#define BOARDCONTROLLER__WDT_DEFAULT_DURATION_VAL 5 //Offset bit when duration > LONG DURATION MIN

#define BOARDCONTROLLER__SLEEP_DURATION_MAX 15000
#define BOARDCONTROLLER__SLEEP_DURATION_MIN 1000
#define BOARDCONTROLLER__SLEEP_MODE 4


///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Setting
///////////////////////////////////////////////////////////////////////////////////////////////////

GlobalSettings_ &SettingSys;

Setting GlobalSettings_::settingArray[GLOBALSETTINGS__MAX_SETTINGS];
uint8_t GlobalSettings_::currentIndex = 0;

struct Setting {
  SETTING_ID id = SETTING_NONE;
  int16_t value = 0;  
};

const int16_t &GlobalSettings_::operator [] (const SETTING_ID settingID) {
  return getSetting(settingID);
}

const int16_t &GlobalSettings_::declareSetting(SETTING_ID id, int16_t initialValue) {
  settingArray[currentIndex].id = id;
  settingArray[currentIndex].value = initialValue;
  currentIndex++;
  return settingArray[currentIndex].value;
}

const int16_t &GlobalSettings_::getSetting(SETTING_ID settingID) {
  Setting *targSetting = findSetting(settingID);
  if (targSetting == nullptr) {
    return 0;
  } else {
    return targSetting->value;
  }
}

const bool GlobalSettings_::setSetting(const SETTING_ID settingID, int16_t newValue) {
  Setting *targSetting = findSetting(settingID);
  if (targSetting == nullptr) {
    return false;
  } else {
    targSetting->value = newValue;
    return true;
  }
}

Setting *GlobalSettings_::findSetting(const SETTING_ID targID) {
  for (int16_t i = 0; i < currentIndex; i++) {
    if (settingArray[i].id == targID) {
      return &settingArray[i];
    }
  }
  return nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Errors
///////////////////////////////////////////////////////////////////////////////////////////////////

GlobalErrors_ &ErrorSys;

uint8_t GlobalErrors_::errorArray[GLOBALERRORS__MAX_ERRORS];
uint8_t GlobalErrors_::currentIndex = 0;
bool GlobalErrors_::restartActive = false;

void GlobalErrors_::report(ERROR_ID errorType, const char *funcName, const char *fileName, int16_t lineNum) {
  if (errorType == 0) { return; }
  // We print errors when in debug mode.
  if (PROG__DEBUG_MODE == 1) {
    printError(errorType, funcName, fileName, lineNum);
  }
  // Do we need to shift value to start overriding?
  if (currentIndex == GLOBALERRORS__MAX_ERRORS) {
    for (int16_t i = 0; i < GLOBALERRORS__MAX_ERRORS - 1; i++) {
      errorArray[i] = errorArray[i + 1];
    }
    errorArray[GLOBALERRORS__MAX_ERRORS - 1] = (uint8_t)errorType;
  } else {
    errorArray[currentIndex] = (uint8_t)errorType;
    currentIndex++;
  }
}

bool GlobalErrors_::assert(bool statement, ERROR_ID errorType, const char *funcName, const char *fileName, int16_t lineNum) {
  if (!statement && !restartActive) {
    if (PROG__DEBUG_MODE == 1) {
      printError(errorType, funcName, fileName, lineNum);
    }
    previousAssert = (uint8_t)errorType;
    // Do we want to reset on false assert?
    if (GLOBALERRORS__ASSERT_RESET == 1) {
      restartActive = true;
      Board.forceRestart();
    }
  }
  return statement; 
}

bool GlobalErrors_::deny(bool statement, ERROR_ID errorType, const char *funcName, const char *fileName, int16_t lineNum) {
  // Invert bools so we restart on statement = true ~AND~ return true.
  return !assert(!statement, errorType, funcName, fileName, lineNum);
}

ERROR_ID GlobalErrors_::getLastError() const {
  if (currentIndex == 0) { return ERROR_NONE; }
    currentIndex--;
    return (ERROR_ID)errorArray[currentIndex];
}

uint8_t GlobalErrors_::getErrorCount() const { return currentIndex; }

ERROR_ID GlobalErrors_::getLastAssert() const { return (ERROR_ID)previousAssert; }

void GlobalErrors_::clearLastAssert() const { previousAssert = 0; }

void GlobalErrors_::printError(ERROR_ID errorType, const char *funcName, const char *fileName, int16_t lineNum) {
  if (!SERIAL_PORT_MONITOR) {
    SERIAL_PORT_MONITOR.begin(0); 
    uint8_t timeoutCount;
    // Wait until serial starts.
    while(!SERIAL_PORT_MONITOR) {
      delay(1);
    }
  }
  // Print string to serial monitor.
  SERIAL_PORT_MONITOR.print("ERROR -> {Error: " + (String)(uint8_t)errorType + "}, {Function: "
    + (String)funcName + "}, {File: " + (String)fileName + "}, {Line: " + (String)lineNum + "}");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Board Controller -> Public Methods
///////////////////////////////////////////////////////////////////////////////////////////////////

BoardController_ &Board;

bool BoardController_::wdtInitialized = false;

volatile int16_t wdtRemainingCycles = 0;
volatile int16_t wdtTotalCycles = 0;
volatile int16_t wdtState = 0; // 0 = off, 1 = restart 2 = no restarts, 3 = sleep
wdtHandler callback = nullptr;
wdtHandler wakeCallback = nullptr;

void BoardController_::forceRestart() {
  __disable_irq();
  NVIC_SystemReset();
}

void BoardController_::setRestartFlag(RESTART_FLAG_ID flag) {
  restartFlag = (uint8_t)flag;
}

RESTART_FLAG_ID BoardController_::getRestartFlag() {
  return (RESTART_FLAG_ID)restartFlag;
}

void clearRestartFlag() {
  restartFlag = (uint8_t)RESTART_NONE;
}

int32_t BoardController_::beginWatchdogTimer(int32_t duration, wdtHandler watchdogSubscriber = nullptr,
  bool restartOnExpire = true) {
    // Check for exceptions
    if (wdtState == 3) return -1; 
    if (wdtState != 0) disableWatchdogTimer();
    // If handler function is provided set it to wdt handler.
    if (watchdogSubscriber != nullptr ) { 
      callback = watchdogSubscriber; 
    } else {
      callback = nullptr;
    }
    if (restartOnExpire) {
      wdtState = 1; // restarts on expire -> on
    } else {
      wdtState = 2; // restart on expire -> off
    }
    uint8_t offsetVal = 0;
    int32_t estimateDuration = 0;
    // Calculate timing & start WDT.
    processDurationWDT(duration, offsetVal, estimateDuration);
    startWDT(offsetVal + 1, offsetVal, false, -1);
    return estimateDuration;
}

void BoardController_::restartWatchdogTimer() {
  //Restart watchdog timer.
  WDT->CLEAR.reg = BOARDCONTROLLER__WDT_DISABLE_KEY;
  while(WDT->CTRLA.reg);
  // Reset cycle counter.
  wdtRemainingCycles = wdtTotalCycles; 
}

void BoardController_::disableWatchdogTimer() { 
  // Disable watchdog timer.
  WDT->CTRLA.bit.ENABLE = 0;
  while(WDT->SYNCBUSY.reg);
  // Reset fields.
  wdtState = 0;
  wdtRemainingCycles = 0;
  wdtTotalCycles = 0;
}

int32_t BoardController_::sleep(int32_t duration, wdtHandler wakeSubscriber) {
  if (wdtState != 0) disableWatchdogTimer();
  if (callback != nullptr) callback = nullptr;
  // We will send these to duration processing method...
  uint8_t offsetVal = 0;
  int32_t estimateDuration = 0;
  // Set state to 3 = sleep & start WDT
  wdtState = 3; 
  processDurationWDT(duration, offsetVal, estimateDuration);
  startWDT(11, 0, true, offsetVal);
  // Actually put board to sleep:
  PM->SLEEPCFG.bit.SLEEPMODE = BOARDCONTROLLER__SLEEP_MODE;
  while(PM->SLEEPCFG.bit.SLEEPMODE != BOARDCONTROLLER__SLEEP_MODE);

  // CODE RESUMED HERE ON WAKE /// // <- I think this is wrong....

  if (wdtState != 0) wdtState = 0;
  wakeSubscriber();
  return estimateDuration;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Board Controller -> Private Methods
///////////////////////////////////////////////////////////////////////////////////////////////////

void BoardController_::initWDT() {
  wdtInitialized = true; // Set flag
  OSC32KCTRL->OSCULP32K.bit.EN1K = 1;
  OSC32KCTRL->OSCULP32K.bit.EN32K = 0;
  // Non-m4 chips dont support dynamic priority changes.
  if (NVIC_GetPriority(WDT_IRQn) != 0) { 
    NVIC_DisableIRQ(WDT_IRQn);  
    NVIC_ClearPendingIRQ(WDT_IRQn);
  }
  // Set up interupt.
  NVIC_SetPriority(WDT_IRQn, 0); 
  NVIC_EnableIRQ(WDT_IRQn);
  // Sync
  while (WDT->SYNCBUSY.reg);

  // Disable USB so we can change configs
  USB->DEVICE.CTRLA.bit.ENABLE = 0;
  while (USB->DEVICE.SYNCBUSY.bit.ENABLE);
  USB->DEVICE.CTRLA.bit.RUNSTDBY = 0; // <- Without this sleep will not work!
  USB->DEVICE.CTRLA.bit.ENABLE = 1;   
  while (USB->DEVICE.SYNCBUSY.bit.ENABLE);
}

void BoardController_::startWDT(uint8_t timeoutVal, uint8_t offsetVal, bool enableWM,
  uint8_t closedIntervalVal = 0) {
  if (BOARDCONTROLLER__ENABLE_WDT == 1) { // Master WDT switch.
    // Have we initialized the WDT?
    if (!wdtInitialized) initWDT();
    // Disable WDT -> lets us change settings.
    WDT->CTRLA.reg = 0; 
    while (WDT->SYNCBUSY.reg);
    // Clear & enable warning.
    WDT->INTFLAG.bit.EW = 1; 
    WDT->INTENSET.bit.EW = 1; 
    //Set WDT timeout
    WDT->CONFIG.bit.PER = timeoutVal;
    // Window mode?
    if (enableWM) {
      WDT->CONFIG.bit.WINDOW = closedIntervalVal;
      WDT->CTRLA.bit.WEN = 1; 
    } else{
      WDT->CTRLA.bit.WEN = 0; 
    }
    while (WDT->SYNCBUSY.reg);
    WDT->EWCTRL.bit.EWOFFSET = offsetVal;
    // Clear then start watchdog
    WDT->CLEAR.reg = BOARDCONTROLLER__WDT_DISABLE_KEY;
    while (WDT->CTRLA.reg);
    WDT->CTRLA.bit.ENABLE = 1;
    while (WDT->SYNCBUSY.reg);
  }
}

void BoardController_::processDurationWDT(int32_t &targDuration, uint8_t &offsetValReturn, 
  int32_t &estimateDurationReturn) {
  // Change functionality if sleep mode = on.
  if (wdtState == 3) {
    if (targDuration <= 1000) {
      offsetValReturn = 7;
      estimateDurationReturn = 1000;
    } else if (targDuration <= 2000) {
      offsetValReturn = 8;
      estimateDurationReturn = 2000;
    } else if (targDuration <= 4000) {
      offsetValReturn = 9;
      estimateDurationReturn = 4000;
    } else if (targDuration <= 8000) {
      offsetValReturn = 10;
      estimateDurationReturn = 8000; 
    } else { 
      offsetValReturn = 11;
      estimateDurationReturn = 16000; // -> Maximum WDT duration -> No cycling here unfortunetly.
    }
  } else {
    // Check for OOB (out of bounds) -> If so correct it.
    if (targDuration < BOARDCONTROLLER__WDT_MIN_DURATION) {
      targDuration = BOARDCONTROLLER__WDT_MIN_DURATION;
    } else if (targDuration > BOARDCONTROLLER__WDT_MAX_DURATION) {
      targDuration = BOARDCONTROLLER__WDT_MAX_DURATION;
    }
    // Map duration onto one of clock divisiors available for WDT timer...
    if (targDuration > BOARDCONTROLLER__WDT_LONG_DURATION_MIN) {
      offsetValReturn = BOARDCONTROLLER__WDT_LONG_DURATION_VAL; 
      wdtTotalCycles = divCeiling(targDuration, BOARDCONTROLLER__WDT_LONG_DURATION_CYCLES);
      estimateDurationReturn = wdtTotalCycles * BOARDCONTROLLER__WDT_LONG_DURATION_CYCLES;
    } else {
      offsetValReturn = BOARDCONTROLLER__WDT_DEFAULT_DURATION_VAL;
      wdtTotalCycles = divCeiling(targDuration, BOARDCONTROLLER__WDT_DEFAULT_DURATION_CYCLES);
      estimateDurationReturn = wdtTotalCycles * BOARDCONTROLLER__WDT_DEFAULT_DURATION_CYCLES;
    }
    // Set remaining = to initial to begin.
    wdtRemainingCycles = wdtTotalCycles;
  } 
}

void WDT_Handler(void) {
  wdtRemainingCycles--;
  if (wdtRemainingCycles <= 0) {
    // Reset values
    wdtRemainingCycles = 0;
    wdtTotalCycles = 0;
    wdtState = 0;
    if (callback != nullptr && wdtState != 3) callback();
    // Do we want to initiate restart?
    if (wdtState == 1) {
      if (callback != nullptr) callback();
      WDT->CLEAR.reg = BOARDCONTROLLER__WDT_DISABLE_KEY - 1; // NOTE -> May not need this?
      while (true); 
    } else {
      // Disable WDT:
      WDT->CTRLA.bit.ENABLE = 0;            
      while (WDT->SYNCBUSY.reg);
      // Clear iterrupt flag:
      WDT->INTFLAG.bit.EW = 1;
    }
  } else { 
    // More cycles left -> clear flag
    WDT->INTFLAG.bit.EW = 1;
    // Reset clock
    WDT->CLEAR.reg = BOARDCONTROLLER__WDT_DISABLE_KEY;
    while(WDT->SYNCBUSY.reg);
  }
}
















