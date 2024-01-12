///////////////////////////////////////////////////////////////////////////////////////////////////
///// FILE -> GLOBAL 
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <inttypes.h>
#include <GlobalDefs.h>

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

class ErrorSys_ {
  protected: 
    ErrorSys_() {}

  public:

    void throwError(ERROR_ID error);

    void assert(bool statement, ASSERT_ID error);



};
extern ErrorSys_ &Error;


