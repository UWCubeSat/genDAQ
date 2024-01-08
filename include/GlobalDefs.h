///////////////////////////////////////////////////////////////////////////////////////////////////
///// FILE -> GLOBAL DEFINITIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <Arduino.h>


#define MIN(A,B)    ({ __typeof__(A) __a = (A); __typeof__(B) __b = (B); __a < __b ? __a : __b; })
#define MAX(A,B)    ({ __typeof__(A) __a = (A); __typeof__(B) __b = (B); __a < __b ? __b : __a; })

#define CLAMP(x, low, high) ({\
  __typeof__(x) __x = (x); \
  __typeof__(low) __low = (low);\
  __typeof__(high) __high = (high);\
  __x > __high ? __high : (__x < __low ? __low : __x);\
})

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> ERROR SYSTEM
///////////////////////////////////////////////////////////////////////////////////////////////////

enum ERROR_ID : uint8_t {
  ERROR_NONE,
  ERROR_I2C_UNKNOWN,
  ERROR_I2C_PINS,
  ERROR_I2C_BUS_BUSY,
  ERROR_I2C_BUS_OTHER,
  ERROR_I2C_DEVICE,
  ERROR_I2C_REGISTER,
  ERROR_I2C_OTHER
};

enum ASSERT_ID : uint8_t {
  ASSSERT_OTHER,
  ASSERT_NULLPTR
};

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> DMA UTILITY
///////////////////////////////////////////////////////////////////////////////////////////////////

//// DMA OTHER SETTINGS ////
#define DMA_MAX_DESCRIPTORS 5
#define DMA_MAX_CHANNELS 16
#define DMA_PRIORITY_LVL_COUNT 4
#define DMA_IRQ_COUNT 5
#define DMA_DESCRIPTOR_VALID_COUNT 3

//// DMA DEFAULT DESCRIPTOR SETTINGS ////
#define DMA_DEFAULT_SOURCE 0
#define DMA_DEFAULT_DESTINATION 0
#define DMA_DEFAULT_TRANSFER_AMOUNT 1
#define DMA_DEFAULT_DATA_SIZE DMAC_BTCTRL_BEATSIZE_BYTE_Val
#define DMA_DEFAULT_STEPSIZE DMAC_BTCTRL_STEPSIZE_X1_Val
#define DMA_DEFAULT_STEPSELECTION DMAC_BTCTRL_STEPSEL_SRC_Val
#define DMA_DEFAULT_TRANSFER_ACTION DMAC_BTCTRL_BLOCKACT_SUSPEND_Val
#define DMA_DEFAULT_INCREMENT_SOURCE 1
#define DMA_DEFAULT_INCREMENT_DESTINATION 1

//// DMA DEFAULT CHANNEL SETTINGS ////
#define DMA_DEFAULT_TRANSMISSION_THRESHOLD 0
#define DMA_DEFAULT_BURST_LENGTH 0
#define DMA_DEFAULT_TRIGGER_ACTION 0
#define DMA_DEFAULT_TRIGGER_SOURCE TRIGGER_SOFTWARE
#define DMA_DEFAULT_RUN_STANDBY 1
#define DMA_DEFAULT_PRIORITY_LVL 1

typedef void (*DMACallbackFunction)(DMA_CALLBACK_REASON reason, DMAChannel &source, 
int16_t descriptorIndex, int16_t currentTrigger, DMA_ERROR error);

enum DMA_TARGET : uint8_t {
  SOURCE,
  DESTINATION
};

enum DMA_TRANSFER_ACTION : uint8_t {
  ACTION_NONE = 0,
  ACTION_BLOCK_INTERRUPT = 1,
  ACTION_SUSPEND = 2,
  ACTION_SUSPEND_BLOCK_INTERRUPT = 3
};

enum DMA_TRIGGER_ACTION : uint8_t {
  ACTION_TRANSMIT_BLOCK = 0,
  ACTION_TRANSMIT_BURST = 2,
  ACTION_TRANSMIT_ALL = 3
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

enum DMA_TRIGGER : int16_t {
  TRIGGER_SOFTWARE          = 0,
  TRIGGER_RTC_TIMESTAMP     = 1,
  TRIGGER_DSU_DCC0          = 2,
  TRIGGER_DSU_DCC1          = 3,
  TRIGGER_SERCOM0_RX        = 4,
  TRIGGER_SERCOM0_TX        = 5,
  TRIGGER_SERCOM1_RX        = 6,
  TRIGGER_SERCOM1_TX        = 7,
  TRIGGER_SERCOM2_RX        = 8,
  TRIGGER_SERCOM2_TX        = 9,
  TRIGGER_SERCOM3_RX        = 10,
  TRIGGER_SERCOM3_TX        = 11,
  TRIGGER_SERCOM4_RX        = 12,
  TRIGGER_SERCOM4_TX        = 13,
  TRIGGER_SERCOM5_RX        = 14,
  TRIGGER_SERCOM5_TX        = 15,
  TRIGGER_SERCOM6_RX        = 16,
  TRIGGER_SERCOM6_TX        = 17,
  TRIGGER_SERCOM7_RX        = 18,
  TRIGGER_SERCOM7_TX        = 19,
  TRIGGER_CAN0_DEBUG_REQ    = 20,
  TRIGGER_CAN1_DEBUG_REQ    = 21,
  TRIGGER_TCC0_OOB          = 22,
  TRIGGER_TCC0_COMPARE_0    = 23,
  TRIGGER_TCC0_COMPARE_1    = 24,
  TRIGGER_TCC0_COMPARE_2    = 25,
  TRIGGER_TCC0_COMPARE_3    = 26,
  TRIGGER_TCC0_COMPARE_4    = 27,
  TRIGGER_TCC0_COMPARE_5    = 28,
  TRIGGER_TCC1_OOB          = 29,
  TRIGGER_TCC1_COMPARE_0    = 30,
  TRIGGER_TCC1_COMPARE_1    = 31,
  TRIGGER_TCC1_COMPARE_2    = 32,
  TRIGGER_TCC1_COMPARE_3    = 33,
  TRIGGER_TCC2_OOB          = 34,
  TRIGGER_TCC2_COMPARE_0    = 35,
  TRIGGER_TCC2_COMPARE_1    = 36,
  TRIGGER_TCC2_COMPARE_2    = 37,
  TRIGGER_TCC3_OOB          = 38,
  TRIGGER_TCC3_COMPARE_0    = 39,
  TRIGGER_TCC3_COMPARE_1    = 40,
  TRIGGER_TCC4_OOB          = 41,
  TRIGGER_TCC4_COMPARE_0    = 42,
  TRIGGER_TCC4_COMPARE_1    = 43,
  TRIGGER_TC0_OOB           = 44,
  TRIGGER_TC0_COMPARE_0     = 45,
  TRIGGER_TC0_COMPARE_1     = 46,
  TRIGGER_TC1_OOB           = 47,
  TRIGGER_TC1_COMPARE_0     = 48,
  TRIGGER_TC1_COMPARE_1     = 49,
  TRIGGER_TC2_OOB           = 50,
  TRIGGER_TC2_COMPARE_0     = 51,
  TRIGGER_TC2_COMPARE_1     = 52,
  TRIGGER_TC3_OOB           = 53,
  TRIGGER_TC3_COMPARE_0     = 54,
  TRIGGER_TC3_COMPARE_1     = 55,
  TRIGGER_TC4_OOB           = 56,
  TRIGGER_TC4_COMPARE_0     = 57,
  TRIGGER_TC4_COMPARE_1     = 58,
  TRIGGER_TC5_OOB           = 59,
  TRIGGER_TC5_COMPARE_0     = 60,
  TRIGGER_TC5_COMPARE_1     = 61,
  TRIGGER_TC6_OOB           = 62,
  TRIGGER_TC6_COMPARE_0     = 63,
  TRIGGER_TC6_COMPARE_1     = 64,
  TRIGGER_TC7_OOB           = 65,
  TRIGGER_TC7_COMPARE_0     = 66,
  TRIGGER_TC7_COMPARE_1     = 67,
  TRIGGER_ADC0_RESRDY       = 68,
  TRIGGER_ADC0_SEQ          = 69,
  TRIGGER_ADC1_RESRDY       = 70,
  TRIGGER_ADC1_SEQ          = 71,
  TRIGGER_DAC_EMPTY0        = 72,
  TRIGGER_DAC_EMPTY1        = 73,
  TRIGGER_DAC_RESULT_READY0 = 74,
  TRIGGER_DAC_RESULT_READY1 = 75,
  TRIGGER_I2S_RX0           = 76,
  TRIGGER_I2S_RX1           = 77,
  TRIGGER_I2S_TX0           = 78,
  TRIGGER_I2S_TX1           = 79,
  TRIGGER_PCC_RX            = 80,
  TRIGGER_AES_WRITE         = 81,
  TRIGGER_AES_READ          = 82,
  TRIGGER_QSPI_RX           = 83,
  TRIGGER_QSPI_TX           = 84
};

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> IO
///////////////////////////////////////////////////////////////////////////////////////////////////

#define IO_MAX_INSTANCES 10

enum IO_TYPE {
  TYPE_UNKNOWN,
  TYPE_I2CSERIAL
};

/////////////////////////////////////////// I2C SERIAL ////////////////////////////////////////////

#define I2C_IRQ_PRIORITY 1
#define I2C_DEFAULT_BAUDRATE 100000
#define I2CBUS_CACHE_SIZE 64

#define I2C_READDESC_DATASIZE 1
#define I2C_READDESC_INCREMENTCONFIG_SOURCE false
#define I2C_READDESC_INCREMENTCONFIG_DEST true
#define I2C_READDESC_TRANSFERACTION ACTION_NONE
#define I2C_READCHANNEL_TRIGGERACTION ACTION_TRANSMIT_BURST
#define I2C_READCHANNEL_BURSTLENGTH 1
#define I2C_READCHANNEL_STANDBYCONFIG false
#define I2C_WRITEDESC_DATASIZE 1
#define I2C_WRITEDESC_INCREMENTCONFIG_SOURCE true
#define I2C_WRITEDESC_INCREMENTCONFIG_DEST false
#define I2C_WRITEDESC_TRANSFERACTION ACTION_NONE
#define I2C_WRITECHANNEL_TRIGGERACTION ACTION_TRANSMIT_BURST
#define I2C_WRITECHANNEL_BURSTLENGTH 1
#define I2C_WRITECHANNEL_STANDBYCONFIG false

enum I2C_STATUS : uint8_t {
  I2C_IDLE,
  I2C_REQUEST_BUSY,
  I2C_REQUEST_COMPLETE,
  I2C_WRITE_BUSY,
  I2C_READ_BUSY,
  I2C_ERROR
};

typedef void (*I2CCallbackFunction)(I2CSerial &sourceInstance, I2C_STATUS currentStatus);

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> SERCOM
///////////////////////////////////////////////////////////////////////////////////////////////////

#define SERCOM_IRQ_COUNT 4
#define SERCOM_FREQ_REF 48000000ul

struct SERCOM_REF_OBJ {
  uint8_t DMAReadTrigger; 
  uint8_t DMAWriteTrigger;
  uint8_t clock;
  IRQn_Type baseIRQ;
};

inline Sercom *GET_SERCOM(int16_t sercomNum) {
  Sercom *s = SERCOM0;
  switch(sercomNum) {
    case 0: s = SERCOM0;
    case 1: s = SERCOM1;
    case 2: s = SERCOM2;
    case 3: s = SERCOM3;
    case 4: s = SERCOM4;
    case 5: s = SERCOM5;
  }
  return s;
}

extern const SERCOM_REF_OBJ SERCOM_REF[] = {
  {SERCOM0_DMAC_ID_RX, SERCOM0_DMAC_ID_TX, SERCOM0_GCLK_ID_CORE, SERCOM0_0_IRQn},
  {SERCOM1_DMAC_ID_RX, SERCOM1_DMAC_ID_TX, SERCOM1_GCLK_ID_CORE, SERCOM1_0_IRQn},
  {SERCOM2_DMAC_ID_RX, SERCOM2_DMAC_ID_TX, SERCOM2_GCLK_ID_CORE, SERCOM2_0_IRQn},
  {SERCOM3_DMAC_ID_RX, SERCOM3_DMAC_ID_TX, SERCOM3_GCLK_ID_CORE, SERCOM3_0_IRQn},
  {SERCOM4_DMAC_ID_RX, SERCOM4_DMAC_ID_TX, SERCOM4_GCLK_ID_CORE, SERCOM4_0_IRQn},
  {SERCOM5_DMAC_ID_RX, SERCOM5_DMAC_ID_TX, SERCOM5_GCLK_ID_CORE, SERCOM5_0_IRQn} 
};







