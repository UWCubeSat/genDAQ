
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


