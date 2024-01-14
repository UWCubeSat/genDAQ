///////////////////////////////////////////////////////////////////////////////////////////////////
///// FILENAME -> GLOBAL 
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <Reset.h>
#include <inttypes.h>
#include <GlobalDefs.h>

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> HELPER funcNameS
///////////////////////////////////////////////////////////////////////////////////////////////////

uint32_t getAddr(void *pointer);

void *getVoid(uint32_t addr);

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


