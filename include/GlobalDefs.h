///////////////////////////////////////////////////////////////////////////////////////////////////
///// FILE -> GLOBAL DEFINITIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <Arduino.h>

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> ENUMS
///////////////////////////////////////////////////////////////////////////////////////////////////

enum ERROR_ID : uint8_t {
  ERROR_NONE,
  ERROR_I2C_UNKNOWN,
  ERROR_I2C_BUS_BUSY,
  ERROR_I2C_DEVICE,
  ERROR_I2C_REGISTER,
  ERROR_I2C_OTHER
};

enum DMA_TARGET : uint8_t {
  SOURCE,
  DESTINATION
};

enum DMA_ACTION : uint8_t {
  NONE = 0,
  BLOCK_INTERRUPT = 1,
  SUSPEND = 2,
  SUSPEND_BLOCK_INTERRUPT = 3
};

enum DMA_STATUS : uint8_t {
  DMA_CHANNEL_BUSY,
  DMA_CHANNEL_SUSPENDED,
  DMA_CHANNEL_ERROR,
  DMA_CHANNEL_IDLE,
  DMA_CHANNEL_DISABLED
};

enum DMA_ERROR : uint8_t {
  DMA_ERROR_NONE,
  DMA_ERROR_TRANSFER,
  DMA_ERROR_CRC,
  DMA_ERROR_DESCRIPTOR,
  DMA_ERROR_UNKNOWN
};

enum DMA_CALLBACK_REASON : uint8_t {
  REASON_SUSPENDED,
  REASON_TRANSFER_COMPLETE,
  REASON_ERROR,
  REASON_UNKNOWN
};

enum DMA_TRIGGER : uint8_t {
  TRIGGER_SOFTWARE = 0,
  TRIGGER_RTC = 1,
  TRIGGER_DSU_DCC0 = 2,
  TRIGGER_DSU_DCC1 = 3,
  TRIGGER_SERCOM0_RX = 4,
  TRIGGER_SERCOM0_TX = 5,
  TRIGGER_SERCOM1_RX = 6,
  TRIGGER_SERCOM1_TX = 7,
  TRIGGER_SERCOM2_RX = 8,
  TRIGGER_SERCOM2_TX = 9,
  TRIGGER_SERCOM3_RX = 10,
  TRIGGER_SERCOM3_TX = 11,
  TRIGGER_SERCOM4_RX = 12,
  TRIGGER_SERCOM4_TX = 13,
  TRIGGER_SERCOM5_RX = 14,
  TRIGGER_SERCOM5_TX = 15,
  TRIGGER_SERCOM6_RX = 16,
  TRIGGER_SERCOM6_TX = 17,
  TRIGGER_SERCOM7_RX = 18,
  TRIGGER_SERCOM7_TX = 19,
  // ADD MORE LATER...
};

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> DMA UTILITY
///////////////////////////////////////////////////////////////////////////////////////////////////

#define DMA_MAX_DESCRIPTORS 5
#define DMA_MAX_CHANNELS 15

#define DMA_DEFAULT_SOURCE 0
#define DMA_DEFAULT_DESTINATION 0
#define DMA_DEFAULT_TRANSFER_AMOUNT 1
#define DMA_DEFAULT_DATA_SIZE DMAC_BTCTRL_BEATSIZE_BYTE_Val
#define DMA_DEFAULT_STEPSIZE DMAC_BTCTRL_STEPSIZE_X1_Val
#define DMA_DEFAULT_STEPSELECTION DMAC_BTCTRL_STEPSEL_SRC_Val
#define DMA_DEFAULT_ACTION DMAC_BTCTRL_BLOCKACT_SUSPEND_Val
#define DMA_DEFAULT_INCREMENT_SOURCE 1
#define DMA_DEFAULT_INCREMENT_DESTINATION 1

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> I2CBus
///////////////////////////////////////////////////////////////////////////////////////////////////

#define I2C_DEFAULT_BAUDRATE 100000

#define I2CBUS_CACHE_SIZE 64

