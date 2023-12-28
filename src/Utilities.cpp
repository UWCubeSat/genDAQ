///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Pre-Processor & General Declarations
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <Utilities.h>

struct Setting;

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Macro Utilities
///////////////////////////////////////////////////////////////////////////////////////////////////

#define THROW(typeID) ErrorSys.report(typeID, __FUNCTION__, __FILE__, __LINE__)
#define ASSERT(statement, typeID) ErrorSys.assert(statement, typeID, __FUNCTION__, __FILE__, __LINE__)
#define DENY(statement, typeID) ErrorSys.deny(statement, typeID, __FUNCTION__, __FILE__, __LINE__)

#define divCeiling(x, y) (!!x + ((x - !!x) / y))


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

ERROR_ID GlobalErrors_::getLastError() {
  if (currentIndex == 0) { return ERROR_NONE; }
    currentIndex--;
    return (ERROR_ID)errorArray[currentIndex];
}

uint8_t GlobalErrors_::getErrorCount() { return currentIndex; }

ERROR_ID GlobalErrors_::getLastAssert() { return (ERROR_ID)previousAssert; }

void GlobalErrors_::clearLastAssert() { previousAssert = 0; }

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
bool BoardController_::itInitialized = false;
bool BoardController_::sleepInitialized = false;

volatile int16_t wdtRemainingCycles = 0;
volatile int16_t wdtTotalCycles = 0;
volatile int16_t wdtState = 0; // 0 = off, 1 = on
volatile uint8_t sleepState = 0; // 0 not sleeping, 1 = sleeping
volatile int16_t itState = 0; // 0 = off, 1 = one
uint8_t itCurrentDuration = 0; // Used for reset of interrupt timer
volatile interruptFunc wdtCallback = nullptr;
volatile interruptFunc itCallback = nullptr;


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

int32_t BoardController_::beginWatchdogTimer(int32_t duration, interruptFunc watchdogSubscriber = nullptr) {
    // Check exceptions
    if (duration > BOARDCONTROLLER__WDT_DURATION_MAX) duration = BOARDCONTROLLER__WDT_DURATION_MAX;
    if (wdtState != 0) disableWatchdogTimer();

    // Add subscriber
    if (watchdogSubscriber != nullptr ) { 
      wdtCallback = watchdogSubscriber; 
    } else wdtCallback = nullptr;

    // Set mode  & calculate cycles.
    wdtState = 1;    
    wdtTotalCycles  = divCeiling(duration, BOARDCONTROLLER__WDT_DURATION_CYCLES);
    wdtRemainingCycles = wdtTotalCycles;

    // Start WDT & return estimate time.
    startWDT(BOARDCONTROLLER__WDT_DURATION_VAL + 1, BOARDCONTROLLER__WDT_DURATION_VAL, false, -1);
    return wdtTotalCycles * BOARDCONTROLLER__WDT_DURATION_CYCLES;
}

void BoardController_::restartWatchdogTimer() {
  //Restart watchdog timer.
  WDT->CLEAR.reg = WDT_CLEAR_CLEAR_KEY_Val;
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

void BoardController_::initSleep() {
  // Set flag
  sleepInitialized = true;

  // Disable USB so we can change configs
  USB->DEVICE.CTRLA.bit.ENABLE = 0;
  while (USB->DEVICE.SYNCBUSY.bit.ENABLE);
  USB->DEVICE.CTRLA.bit.RUNSTDBY = 0; // <- Without this sleep will not work!
  USB->DEVICE.CTRLA.bit.ENABLE = 1;   
  while (USB->DEVICE.SYNCBUSY.bit.ENABLE);
}



void BoardController_::initWDT() {
  //Set flag
  wdtInitialized = true; 
  
  // Start low power clocks -> 1024hz w 1:32 prescale
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
  while (WDT->SYNCBUSY.reg); // Sync
}

void BoardController_::startWDT(uint8_t timeoutVal, uint8_t offsetVal, bool enableWM,
  uint8_t closedIntervalVal = 0) {
  // check master WDT switch.
  if (BOARDCONTROLLER__ENABLE_WDT == 1) { 
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
    // Set warning offset cycle duration.
    while (WDT->SYNCBUSY.reg);
    WDT->EWCTRL.bit.EWOFFSET = offsetVal;

    // Clear then start watchdog
    WDT->CLEAR.reg = WDT_CLEAR_CLEAR_KEY_Val;
    while (WDT->CTRLA.reg);
    WDT->CTRLA.bit.ENABLE = 1;
    while (WDT->SYNCBUSY.reg);
  }
}

// #IRQ
void WDT_Handler(void) {
  wdtRemainingCycles--;
  if (wdtRemainingCycles <= 0) {
    // Initiate restart 
    if (wdtCallback != nullptr && wdtState != 2) wdtCallback();
    WDT->CLEAR.reg = WDT_CLEAR_CLEAR_KEY_Val - 1; 
    while (true); 
  } else { 
    // Clear timer & cycle again 
    WDT->INTFLAG.bit.EW = 1;
    WDT->CLEAR.reg = WDT_CLEAR_CLEAR_KEY_Val;
    while(WDT->SYNCBUSY.reg);
  }
}

extern "C" char* sbrk(int incr);
int32_t BoardController_::getFreeRam() {
  char top;
  return &top - reinterpret_cast<char*>(sbrk(0));
}

void BoardController_::beginInterruptTimer(int32_t duration, interruptFunc subscriber) {
  // Set config flags
  itCallback = subscriber;

  // Enable clock & disable tc3 so config can be changed.
  GCLK->PCHCTRL[TC3_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK1_Val | (1 << GCLK_PCHCTRL_CHEN_Pos);
  while(GCLK->SYNCBUSY.reg);
  TC3->COUNT16.CTRLA.bit.ENABLE = 0;

  // Enable running while in sleep mode.
  TC3->COUNT16.CTRLA.bit.RUNSTDBY = 1; 

  // Config match mode
  TC3->COUNT16.WAVE.bit.WAVEGEN = TC_WAVE_WAVEGEN_MFRQ;
  while(TC3->COUNT16.SYNCBUSY.reg);
  TC3->COUNT16.INTENSET.reg = 0;
  TC3->COUNT16.INTENSET.bit.MC0 = 1;

  //Enable interrupt
  if (!itInitialized) { // May be able to expand this to include other components...
    itInitialized = true;
    NVIC_EnableIRQ(TC3_IRQn); 
  }

  // Calculate/setup prescalers -> CREDIT: DENIS-VAN-GILS on GITHUB
  int prescaler;
  uint32_t TC_CTRLA_PRESCALER_DIVN;
  TC3->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE;
  while(TC3->COUNT16.SYNCBUSY.reg);
  TC3->COUNT16.CTRLA.reg &= ~TC_CTRLA_PRESCALER_DIV1024;
  while(TC3->COUNT16.SYNCBUSY.reg);
  TC3->COUNT16.CTRLA.reg &= ~TC_CTRLA_PRESCALER_DIV256;
  while(TC3->COUNT16.SYNCBUSY.reg);
  TC3->COUNT16.CTRLA.reg &= ~TC_CTRLA_PRESCALER_DIV64;
  while(TC3->COUNT16.SYNCBUSY.reg);
  TC3->COUNT16.CTRLA.reg &= ~TC_CTRLA_PRESCALER_DIV16;
  while(TC3->COUNT16.SYNCBUSY.reg);
  TC3->COUNT16.CTRLA.reg &= ~TC_CTRLA_PRESCALER_DIV4;
  while(TC3->COUNT16.SYNCBUSY.reg);
  TC3->COUNT16.CTRLA.reg &= ~TC_CTRLA_PRESCALER_DIV2;
  while(TC3->COUNT16.SYNCBUSY.reg);
  TC3->COUNT16.CTRLA.reg &= ~TC_CTRLA_PRESCALER_DIV1;
  while(TC3->COUNT16.SYNCBUSY.reg);

  if (duration > 300000) {
    TC_CTRLA_PRESCALER_DIVN = TC_CTRLA_PRESCALER_DIV1024;
    prescaler = 1024;
  } else if (80000 < duration) {
    TC_CTRLA_PRESCALER_DIVN = TC_CTRLA_PRESCALER_DIV256;
    prescaler = 256;
  } else if (20000 < duration) {
    TC_CTRLA_PRESCALER_DIVN = TC_CTRLA_PRESCALER_DIV64;
    prescaler = 64;
  } else if (10000 < duration) {
    TC_CTRLA_PRESCALER_DIVN = TC_CTRLA_PRESCALER_DIV16;
    prescaler = 16;
  } else if (5000 < duration) {
    TC_CTRLA_PRESCALER_DIVN = TC_CTRLA_PRESCALER_DIV8;
    prescaler = 8;
  } else if (2500 < duration) {
    TC_CTRLA_PRESCALER_DIVN = TC_CTRLA_PRESCALER_DIV4;
    prescaler = 4;
  } else if (1000 < duration) {
    TC_CTRLA_PRESCALER_DIVN = TC_CTRLA_PRESCALER_DIV2;
    prescaler = 2;
  } else {
    TC_CTRLA_PRESCALER_DIVN = TC_CTRLA_PRESCALER_DIV1;
    prescaler = 1;
  }
  // calculate comparator -> CREDIT: DENIS VAN GILES on GITHUB
  int compareValue = (int)(48000000 / (prescaler/((float)duration / 1000000))) - 1;

  // Enable prescaler
  TC3->COUNT16.CTRLA.reg |= TC_CTRLA_PRESCALER_DIVN;
  while(TC3->COUNT16.SYNCBUSY.reg);

  // Set comparator value 
  TC3->COUNT16.COUNT.reg = map(TC3->COUNT16.COUNT.reg, 0, TC3->COUNT16.CC[0].reg, 0, compareValue);
  TC3->COUNT16.CC[0].reg = compareValue;
  while(TC3->COUNT16.SYNCBUSY.reg);

  // Enable the interrup
  TC3->COUNT16.CTRLA.bit.ENABLE = 1;
  while(TC3->COUNT16.SYNCBUSY.reg);
}

void BoardController_::restartInterruptTimer() { beginInterruptTimer(itCurrentDuration, itCallback); }

void BoardController_::disableInterruptTimer() {
  // Disable the timer.
  TC3->COUNT16.CTRLA.bit.ENABLE = 0;
  while(TC3->COUNT16.SYNCBUSY.reg);
  // Set the state variable to 0.
  itState = 0;
}

void TC3_Handler(void) {
  // Only one if called from timeout
  if (TC3->COUNT16.INTFLAG.bit.MC0 == 1) {
    TC3->COUNT16.INTFLAG.bit.MC0 = 1;
  }
  // Call subscriber & reset state.
  itCallback();
  itState = 0;
}

void BoardController_::sleep(uint8_t sleepMode, int32_t duration = 0, void (*wakeSubscriber)(bool timeoutExpire)) {
  // Setup for sleep
  if (duration <= 0) sleepMode = 3;
  if (wdtState != 0) disableWatchdogTimer();
  if (itState != 0) disableInterruptTimer();
  if (sleepInitialized) initSleep();
  sleepState = sleepMode;
  
  // Find corresponding sleep mode
  uint8_t sleepVal = 0;
  if (sleepMode == 1) { // IDLE sleep mode
    sleepVal = PM_SLEEPCFG_SLEEPMODE_IDLE2_Val;
  } else if (sleepMode == 2) { // STANDBY sleep mode
    sleepVal = PM_SLEEPCFG_SLEEPMODE_STANDBY_Val;
    PM->STDBYCFG.bit.FASTWKUP = 3;
  } else { // HYBERNATE sleep mode
    sleepVal = PM_SLEEPCFG_SLEEPMODE_HIBERNATE_Val;
  }

  // If applicable set "wakeup" interrupt
  if ((sleepMode == 1 || sleepMode == 2) && duration > 0) {
    beginInterruptTimer(duration, nullptr);
  }
  // Begin sleep
  PM->SLEEPCFG.bit.SLEEPMODE = sleepVal;
  while(PM->SLEEPCFG.bit.SLEEPMODE != sleepVal);
  __DSB(); 
  __WFI();

  // Code resumed here on wake.
  sleepState = 0;
  if (wakeSubscriber != nullptr) {
    wakeSubscriber(wdtState == 0);
    wakeSubscriber = nullptr;
  }
}























