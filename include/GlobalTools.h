///////////////////////////////////////////////////////////////////////////////////////////////////
///// FILENAME -> GLOBAL 
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <Reset.h>
#include <inttypes.h>
#include <GlobalDefs.h>

// IF EITHER = TRUE, APP IS ERASED ON RESET
// FORCED BY EITHER ASSERT OR RAM ERROR
#define ERRSYS_ERASE_ON_RESET false
#define ERRSYS_BOOTMODE_ON_RESET true

template<typename T> class Setting;
template<typename T> struct SettingCtrl; 

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> SETTING TEMPLATE
///////////////////////////////////////////////////////////////////////////////////////////////////

enum SETTING_TYPE : uint8_t {
  TYPE_VALUE,
  TYPE_REFERENCE,
  TYPE_POINTER
};

enum SETTING_CONFIG : uint8_t {
  SETTING,
  REQUIRE_CLAMP,
  REQUIRE_CEIL,
  REQUIRE_FLOOR,
  RANGE_WRAP
};

template<typename T> class Setting {

  public:

    constexpr Setting(T min, T max, uint8_t clampConfig) {
      
    }

    constexpr Setting(T *set, uint16_t setLength, bool copySet = false) {

    }

    constexpr Setting() {
      
    } 

    Setting() {
      static_assert(decltype(T) == decltype(true));
      initFields();
      valueValid = true;
    }

    bool set(const T setValue) {
      value = T();
      if (set != nullptr) {

        for (int16_t i = 0; i < setLength; i++) {
          if (value == validSet[i]) {

            valueValid = true;
            value = setValue;
            break;
          }
        }
      } else if (min != max) {
        valueValid = false;

        if (setValue > max) {
          valueValid = (bool)setLength;
          value = valueValid ? max : T();

        } else if (setValue < min) {
          valueValid = (bool)setLength;
          value = valueValid ? min : T();

        } else {
          valueValid = true;
          value = setValue;
        }
      } else {
        valueValid = true;
        value = setValue;
      }
      return valueValid;
    }

    T get(void) { return value; }

    const T &getRef(void) { return value; }

    const T *getPtr(void) { return &value; }

    bool isValid(void) { return valueValid; }

    void clear() {
      valueValid = false;
      value = T();
    }

    //// Operator Overloads ////
    bool operator = (const T setValue) { return set(setValue); }

    T operator = (void) { return get(); }

    const T &operator = (void) { return get(); }

    const T *operator = (void) { return getPtr(); }

    operator bool() cosnt { return valueValid; }

  private:
    friend SettingCtrl;
    T value;
    T *valuePtr;
    bool valueValid;

    T min;
    T max;
    bool denyRange;

    T *validSet;
    T *invalidSet;
    uint16_t validSetLength;
    uint16_t invalidSetLength;

    uint8_t requiredType;
    bool nullEnabled;
    
    REQUIRE_CONFIG requireConfig;
    bool enabled;

    void initFields() {
      this->value = T();
      this->valueValid = false;
      this->min = T();
      this->max = T();
      this->set = nullptr;
      this->setLength = 0;
    }

    //// CTRL METHODS ////

    bool requireRange(T min, T max, RANGE_CONFIG rangeConfig) {
      if (!comparable() || min == max) {
        return false;
      }
      this->min = min;
      this->max = max;
      return true;
    }

    bool requireMatch(T *set, uint16_t setLength, bool copySet) {
      if (setLength == 0 || set == nullptr) return false;

      if (copySet) {
        memcpy(validSet, set, sizeof(T) * setLength);
      } else {
        validSet = &set;
      }
      return true;
    }

    void requireType(SETTING_TYPE type) {
      this->requiredType = (uint8_t)type;
    }

    bool setRequireConfig(REQUIRE_CONFIG requireConfig) {
      if (requireConfig && !comparable()) return false;
      this->clamp = (uint8_t)clampConfig;
      return true;
    }
    
    void disableChanges() { enabled = false; }

    void enableChanges() { enabled = true; }


    void comparable() { // Checks to see if comparison opp are implemented for type
      try {
        bool test = min < max;
        test = min > max;
        test = min <= max;
        test = min >= max; 
      } catch {
        return false;
      }
      return true;
    }

};

template<typename T> struct SettingCtrl {
  const T &attachedSetting;
  SettingCtrl (T &Setting) : setting(setting) {};
  T &operator->() const { return attachedSetting; }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> TIMEOUT SYSTEM
///////////////////////////////////////////////////////////////////////////////////////////////////

class Timeout {
  public:
    Timeout(uint32_t timeout, bool start);

    void start(uint32_t timeout, bool restart);
    uint32_t start(bool restart);

    bool isActive();

    void stop();

    uint32_t pause();

    void resume();

    void setTimeout(uint32_t timeout);

    uint32_t total();

    uint32_t remaining();

    bool triggered();

    uint32_t operator = (uint32_t value);

    uint32_t operator++(int);

    operator uint32_t();

    operator bool();

  private:
    bool active;
    bool toFlag;
    uint32_t timeout;
    uint32_t reference;
    uint32_t pauseTime;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> ERROR SYSTEM
///////////////////////////////////////////////////////////////////////////////////////////////////

#define ASSERT(statement, type) Error.assert(statement, type, __LINE__, __FUNC__, __FILE__)
#define DENY(statement, type) Error.deny(statement, type __LINE__, __FUNC__, __FILE__)
#define THROW(type) Error.throwError(type, __LINE__, __FUNC__, __FILE__)

class ErrorSys_ {
    ErrorSys_() {}
    

  public:

    void throwError(ERROR_ID error, int16_t lineNum, const char *funcName, 
                    const char *fileName);

    void assert(bool statement, ERROR_ID type, int16_t lineNum, const char *funcName, 
                const char *fileName);

    void deny(bool statement, ERROR_ID error, int16_t lineNum, const char *funcName, 
              const char *fileName);

    ////// TO DO -> ADD A WAY TO ACCESS THE THROWN ERRORS
    ////// TO DO -> ADD A WAY TO SAVE ASSERT TYPE ACROSS SYS RESETS

  protected:

    struct Error {
      ERROR_ID id;
      int16_t line;
      const char *funcName;
      const char *fileNameNameName;
    };

    static Error ErrorArray[ERRSYS_MAX_ERRORS];
    static int16_t eaReadIndex;
    static int16_t eaWriteIndex;

    void printError(ERROR_ID error, int16_t lineNum, const char *funcName, 
              const char *fileName);

};
extern ErrorSys_ &Error;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> SMART EEPROM MANAGER
///////////////////////////////////////////////////////////////////////////////////////////////////

class EEPROMManager_ {

  public:

    bool initialize(uint32_t minBytes, bool restartNow);

    bool isInitialized();

    bool allocateBlock();

    bool isBusy();

    bool clear();

    bool end();

  private:
    EEPROMManager_() {}

    uint32_t totalSize;
    uint16_t blockSize;
    bool instanceInit;

    bool initialized(); // TO DO 

};

extern EEPROMManager_ &EEPROM;

