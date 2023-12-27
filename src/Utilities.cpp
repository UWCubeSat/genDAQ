///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Pre-Processor & General Declarations
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <Utilities.h>

struct Setting;

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Setting
///////////////////////////////////////////////////////////////////////////////////////////////////

GlobalSettings_ &SettingSys;

Setting GlobalSettings_::settingArray[SETTINGS_AMOUNT_MAX];
uint8_t GlobalSettings_::currentIndex = 0;

struct Setting {
  SETTING_ID id = SETTING_NULL;
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

uint8_t GlobalErrors_::errorArray[ERRORS_AMOUNT_MAX];
uint8_t GlobalErrors_::currentIndex = 0;
bool GlobalErrors_::restartActive = false;

void GlobalErrors_::report(ERROR_TYPE errorType, const char *funcName, const char *fileName, int16_t lineNum) {
  if (errorType == 0) { return; }
  // We print errors when in debug mode.
  if (GEN_DEBUG_MODE == 1) {
    printError(errorType, funcName, fileName, lineNum);
  }
  // Do we need to shift value to start overriding?
  if (currentIndex == ERRORS_AMOUNT_MAX) {
    for (int16_t i = 0; i < ERRORS_AMOUNT_MAX - 1; i++) {
      errorArray[i] = errorArray[i + 1];
    }
    errorArray[ERRORS_AMOUNT_MAX - 1] = (uint8_t)errorType;
  } else {
    errorArray[currentIndex] = (uint8_t)errorType;
    currentIndex++;
  }
}

bool GlobalErrors_::assert(bool statement, ERROR_TYPE errorType, const char *funcName, const char *fileName, int16_t lineNum) {
  if (!statement && !restartActive) {
    if (GEN_DEBUG_MODE == 1) {
      printError(errorType, funcName, fileName, lineNum);
    }
    previousAssert = (uint8_t)errorType;
    // Do we want to reset on false assert?
    if (ERRORS_ASSERT_RESET == 1) {
      restartActive = true;
      ///////////////////// TO COMPLETE -> RESTART PROG
    }
  }
  return statement; 
}

bool GlobalErrors_::deny(bool statement, ERROR_TYPE errorType, const char *funcName, const char *fileName, int16_t lineNum) {
  // Invert bools so we restart on statement = true ~AND~ return true.
  return !assert(!statement, errorType, funcName, fileName, lineNum);
}

ERROR_TYPE GlobalErrors_::getLastError() const {
  if (currentIndex == 0) { return ERR_NULL; }
    currentIndex--;
    return (ERROR_TYPE)errorArray[currentIndex];
}

uint8_t GlobalErrors_::getErrorCount() const { return currentIndex; }

ERROR_TYPE GlobalErrors_::getLastAssert() const { return (ERROR_TYPE)previousAssert; }

void GlobalErrors_::clearLastAssert() const { previousAssert = 0; }

void GlobalErrors_::printError(ERROR_TYPE errorType, const char *funcName, const char *fileName, int16_t lineNum) {
  ///////////////////// TO COMPLETE -> PRINT MESSAGE
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Board Controller
///////////////////////////////////////////////////////////////////////////////////////////////////

BoardController_ &Board;

void BoardController_::restartProgram(BOARD_RESTART_MSG reason) {
  restartMsg = reason;
  __disable_irq();
  NVIC_SystemReset();
};










