
#include <GlobalTools.h>



ErrorSys_ &Error;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> TIMEOUT SYSTEM
///////////////////////////////////////////////////////////////////////////////////////////////////

Timeout::Timeout(uint32_t timeout, bool start) {
  if (timeout == 0) this->timeout = TO_DEFAULT_TIMEOUT;
  this->timeout = timeout;
  toFlag = false;
  reference = 0;
  active = false;

  if (start) this->start(true);
}

uint32_t Timeout::start(bool restart) {
  if (restart || !active) {
    active = true;
    toFlag = false;
    reference = micros();
  } else {
    remaining();
  }
  return timeout;
}

void Timeout::start(uint32_t timeout, bool restart) {
  this->timeout = timeout;
  start(restart);
}

bool Timeout::isActive() {
  return active;
}

void Timeout::stop() {
  active = false;
  reference = 0;
}

uint32_t Timeout::pause() {
  if (!active) return remaining();

  pauseTime = micros() - reference;
  return remaining();
}

void Timeout::resume() {
  if (pauseTime == 0 || !active) return;
  reference = micros() - pauseTime;
  pauseTime = 0;
}

void Timeout::setTimeout(uint32_t timeout) {
  this->timeout = timeout;
}

uint32_t Timeout::remaining() {
  if (pauseTime != 0) return timeout - pauseTime;
  if (!active) return timeout;

  uint32_t to = timeout - (micros() - reference);

  if (micros() - reference < 0) {  // Micros() overflows every ~70min
    reference = micros();
  }

  if (to <= 0) {
    toFlag = true;
    stop();
    return 0;
  } else {
    return timeout - (micros() - reference);
  }
}

uint32_t Timeout::total() {
  return timeout;
}

bool Timeout::triggered() {
  return toFlag;
}

uint32_t Timeout::operator=(uint32_t value) {
  if (value == 0) {
    stop();
  } else {
    timeout = value;
  }
  return remaining();
}

uint32_t Timeout::operator++(int) {
  return remaining();
} 

Timeout::operator uint32_t() {
  return remaining();
}

Timeout::operator bool() {
  return triggered();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> PIN MANAGER (NOT FINISHED)
///////////////////////////////////////////////////////////////////////////////////////////////////

PinManager_ &PinManager;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> ERROR SYSTEM (NOT FINISHED)
///////////////////////////////////////////////////////////////////////////////////////////////////

void ErrorSys_::assert(bool statement, ERROR_ID type, int16_t lineNum, 
  const char *function, const char *file) { 
  
  if (statement) return;
  if (!ERRSYS_ASSERTS_ENABLED) return;

  if (DEBUG_MODE && SERIAL_PORT_MONITOR) {
    printError(type, lineNum, function, file);
  }
  if (ERRSYS_ASSERT_RESET_ENABLED) {
    pinMode(ERRSYS_ASSERT_LED_PRIMARY, OUTPUT);
    pinMode(ERRSYS_ASSERT_LED_SECONDARY, OUTPUT);
    digitalWrite(ERRSYS_ASSERT_LED_PRIMARY, HIGH);

    for (int16_t i = 0; i < ERRSYS_ASSERT_LED_PULSES; i++) {
      digitalWrite(ERRSYS_ASSERT_LED_SECONDARY, HIGH);
      delay(ERRSYS_ASSERT_LED_DELAY_MS);
      digitalWrite(ERRSYS_ASSERT_LED_SECONDARY, LOW);
      delay(ERRSYS_ASSERT_LED_DELAY_MS);
    }
    digitalWrite(ERRSYS_ASSERT_LED_PRIMARY, LOW);
  }

  if (ERRSYS_ASSERT_RESET_ENABLED) {
    __DMB;
    NVIC_SystemReset();
    while(true);
  }
}

void ErrorSys_::deny(bool statement, ERROR_ID type, int16_t lineNum, 
  const char *function, const char *file) {
  return assert(!statement, type, lineNum, function, file);
}

void ErrorSys_::throwError(ERROR_ID type, int16_t lineNum, 
  const char *function, const char *file) {

  eaWriteIndex = (eaWriteIndex == ERRSYS_MAX_ERRORS - 1 ? 0 : eaWriteIndex++);
  if (eaWriteIndex == eaReadIndex) eaReadIndex++;
  ErrorArray[eaWriteIndex] = {type, lineNum, function, file};

  if (DEBUG_MODE) {
    printError(type, lineNum, function, file);
  }
  if (ERRSYS_ERROR_LED_ENABLED) {
    pinMode(ERRSYS_ERROR_LED, OUTPUT);
    digitalWrite(ERRSYS_ERROR_LED, HIGH);
    delay(ERRSYS_ERROR_LED_DELAY_MS);
    digitalWrite(ERRSYS_ERROR_LED, LOW);
    pinMode(ERRSYS_ERROR_LED, OUTPUT);
  }
}

void ErrorSys_::printError(ERROR_ID error, int16_t lineNum, const char *funcName, 
  const char *fileName) {
  if (!SERIAL_PORT_MONITOR) {
    SERIAL_PORT_MONITOR.begin(0);
    bool timeout = false;
    for (int16_t i = 0; i <= ERRSYS_SERIAL_TIMEOUT; i++) {
      delay(1);
      if (SERIAL_PORT_MONITOR) break;
      if (i == ERRSYS_SERIAL_TIMEOUT) return;
    }
  }
  SERIAL_PORT_MONITOR.print("ASSERT TRIGGERED -> ");
  SERIAL_PORT_MONITOR.print("{ Type: " + (String)((uint8_t)error) + "}, ");
  SERIAL_PORT_MONITOR.print("{ Line: " + (String)lineNum + "}, ");
  SERIAL_PORT_MONITOR.print("{ Function: " + (String)funcName + "}, ");
  SERIAL_PORT_MONITOR.print("{ File: " + (String)fileName + "}");
  SERIAL_PORT_MONITOR.println();
}


