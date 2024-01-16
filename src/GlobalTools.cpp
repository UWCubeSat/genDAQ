
#include <GlobalTools.h>

// Singletons
ErrorSys_ &Error;
EEPROMManager_ &EEPROM;

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

__attribute__ ((long_call, section (".ramfunc")))
static void eraseProg(bool enterBootMode) {
    __disable_irq();

  // Erase program by entering boot mode
  if (enterBootMode) {
    #define BOOT_REG 0xf01669efUL;
    unsigned long *trig = (unsigned long*)(HSRAM_ADDR + HSRAM_SIZE - 4);
    *trig = BOOT_REG; // <- This erases prog & enters boot mode
  
  // Erase program with NVMC
  } else {
    #if defined(__SAMD51__) 
      while(!NVMCTRL->STATUS.bit.READY);
      NVMCTRL->ADDR.reg  = 0x00004004;
      NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_CMD_EB | NVMCTRL_CTRLB_CMDEX_KEY;
    #endif
  }
  NVIC_SystemReset();
  while(true);
}

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

  // Are resets enabled?
  if (ERRSYS_ASSERT_RESET_ENABLED) {

    if (ERRSYS_ERASE_ON_RESET) {
      eraseProg(false);

    } else if (ERRSYS_BOOTMODE_ON_RESET) {
      eraseProg(true);

    } else { // Restart program, dont erase
      __disable_irq();                  
      NVIC_SystemReset();              
    }
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

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> SMART EEPROM MANAGER
///////////////////////////////////////////////////////////////////////////////////////////////////

#define SEE_REF_MBYTES 0
#define SEE_REF_BCOUNT 1
#define SEE_REF_PCOUNT 2
#define SEE_DEFAULT_BLOCKS 5
#define SEE_DEFAULT_PAGES 256
#define SEE_MAX_WRITE 16      // Quad-Word

static uint8_t UP[FLASH_USER_PAGE_SIZE] = { 0 }; // User Page

const uint32_t SEE_REF[8][3] {
  {512, 1, 4},
  {1024, 1, 8},
  {2048, 1, 16},
  {4096, 1, 32},
  {8192, 2, 64},
  {16384, 3, 128},
  {32768, 5, 256},
  {65536, 10, 512}
};

bool EEPROMManager_::initialize(uint32_t minBytes, bool restartNow) {
  uint8_t blockCount = 0;
  uint16_t pageCount = 0;

  // If not initialized -> get user page
  if (!instanceInit) {
    instanceInit = true;
    memcpy(UP, (uint8_t *const)NVMCTRL_USER, FLASH_USER_PAGE_SIZE);
  }

  // Determine block & page count
  for (int16_t i = 0; i < (sizeof(SEE_REF[0]) / sizeof(SEE_REF[0][0])); i++) {
    if (SEE_REF[i][SEE_REF_MBYTES] >= minBytes) {
      blockCount = SEE_REF[i][SEE_REF_BCOUNT];
      pageCount = SEE_REF[i][SEE_REF_PCOUNT];
      break;
    }
  }
  if (!blockCount || pageCount) {
    blockCount = SEE_DEFAULT_BLOCKS;
    pageCount = SEE_DEFAULT_PAGES;
  } 

  while(!NVMCTRL->STATUS.bit.READY);
  NVMCTRL->CTRLA.bit.WMODE = NVMCTRL_CTRLA_WMODE_MAN_Val;

  UP[NVMCTRL_FUSES_SEEPSZ_ADDR - NVMCTRL_USER] = 0;
  UP[NVMCTRL_FUSES_SEEPSZ_ADDR - NVMCTRL_USER] = (NVMCTRL_FUSES_SEESBLK(blockCount) 
    | NVMCTRL_FUSES_SEEPSZ((uint8_t)(log2(pageCount) - 2)));

  for (int16_t i = 0; i < FLASH_USER_PAGE_SIZE; i += SEE_MAX_WRITE) {
    memcpy((uint8_t *const)(NVMCTRL_USER + i), UP + i, SEE_MAX_WRITE);
    NVMCTRL->ADDR.bit.ADDR = (uint32_t)(NVMCTRL_USER + i);
    NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_CMD_WQW | NVMCTRL_CTRLB_CMDEX_KEY;
    while(!NVMCTRL->STATUS.bit.READY);
  }
  // Restart if specified
  if (restartNow) {
    __disable_irq();
    __DSB();
    NVIC_SystemReset();
    while(true);
  } else {
    return true;
  }
}


bool EEPROMManager_::isInitialized() { return NVMCTRL->SEESTAT.bit.SBLK; }


