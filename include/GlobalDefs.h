
#pragma once
#include <Arduino.h>

///////////////////////////////////////////////////////////////////////////////////

#define MIN(A,B)    ({ __typeof__(A) __a = (A); __typeof__(B) __b = (B); __a < __b ? __a : __b; })
#define MAX(A,B)    ({ __typeof__(A) __a = (A); __typeof__(B) __b = (B); __a < __b ? __b : __a; })

#define CLAMP(x, low, high) ({\
  __typeof__(x) __x = (x); \
  __typeof__(low) __low = (low);\
  __typeof__(high) __high = (high);\
  __x > __high ? __high : (__x < __low ? __low : __x);\
  })

///////////////////////////////////////////////////////////////////////////////////

//// DMA IMPLEMENTATION SETTINGS ////
#define DMA_MAX_CHANNELS 10
#define DMA_MAX_TASKS 4
#define DMA_IRQ_COUNT 5
#define DMA_PRIORITY_LVL_COUNT 4
#define DMA_MAX_CYCLES 100

//// DMA DEFAULT CONFIGS - DESCRIPTOR ////
#define DMA_DEFAULT_INCREMENT_MODIFIER 1            // STEP SIZE 
#define DMA_DEFAULT_INCREMENT_MODIFIER_TARGET 1     // STEP SELECTION
#define DMA_DEFAULT_INCREMENT_DESTINATION 0         // INCREMENT DESTINATION ENABLE
#define DMA_DEFAULT_INCREMENT_SOURCE 0              // INCREMENT SOURCE ENABLE
#define DMA_DEFAULT_ELEMENT_SIZE 0                  // BEAT SIZE
#define DMA_REQUIRED_TRANSFER_ACTION 0              // BLOCK ACTION 
#define DMA_DEFAULT_ELEMENT_TRANSFER_COUNT 1        // BLOCK TRANSFER COUNT
#define DMA_DEFAULT_EVENT_OUTPUT 0                  // EVENT OUTPUT SELECTION

//// DMA DEFAULT CONFIGS - CHANNEL ////
#define DMA_DEFAULT_TRANSMISSION_THRESHOLD 0 // THRESHOLD - CHCTRLA
#define DMA_DEFAULT_BURST_SIZE 0             // BURST LENGTH - CHCTRLA
#define DMA_DEFAULT_TRIGGER_ACTION 0         // TRIGGER ACTION - CHCTRLA
#define DMA_DEFAULT_TRIGGER_SOURCE 0         // TRIGGER SOURCE - CHCTRLA
#define DMA_DEFAULT_SLEEP_CONFIG 1           // RUN IN STANDBY - CHCTRLA
#define DMA_DEFAULT_PRIORITY_LEVEL 0         // PRIORITY LEVEL - CHPRILVL

//// DMA DEFAULT DESCRIPTOR REG MASK //// 
#define DMA_DEFAULT_DESCRIPTOR_MASK                                                             \
    DMAC_BTCTRL_STEPSIZE(DMA_DEFAULT_INCREMENT_MODIFIER)                                        \
  | (DMAC_BTCTRL_STEPSEL & (DMA_DEFAULT_INCREMENT_MODIFIER_TARGET << DMAC_BTCTRL_STEPSEL_Pos))  \
  | (DMAC_BTCTRL_DSTINC  & (DMA_DEFAULT_INCREMENT_DESTINATION << DMAC_BTCTRL_DSTINC_Pos))       \
  | (DMAC_BTCTRL_SRCINC  & (DMA_DEFAULT_INCREMENT_SOURCE << DMAC_BTCTRL_SRCINC_Pos))            \
  | DMAC_BTCTRL_BEATSIZE(DMA_DEFAULT_ELEMENT_SIZE)                                              \
  | DMAC_BTCTRL_BLOCKACT(DMA_REQUIRED_TRANSFER_ACTION)                                          \
  | DMAC_BTCTRL_EVOSEL(DMA_DEFAULT_EVENT_OUTPUT)


///////////////////////////////////////////////////////////////////////////////////

enum DMA_CHANNEL_STATUS : int16_t {
  CHANNEL_STATUS_NULL   = 0,
  CHANNEL_STATUS_IDLE   = 1,
  CHANNEL_STATUS_PAUSED = 2,
  CHANNEL_STATUS_BUSY   = 3
};

enum DMA_CHANNEL_ERROR : int16_t {
  CHANNEL_ERROR_NONE             = 0,
  CHANNEL_ERROR_DESCRIPTOR_ERROR = 1,
  CHANNEL_ERROR_CRC_ERROR        = 2,
  CHANNEL_ERROR_TRANSFER_ERROR   = 3
};

enum DMA_INTERRUPT_REASON : int16_t {
  SOURCE_TASK_COMPLETE    = 0,
  SOURCE_CHANNEL_FREE     = 1,
  SOURCE_CHANNEL_PAUSED   = 2,
  SOURCE_DESCRIPTOR_ERROR = 3,
  SOURCE_TRANSFER_ERROR   = 4,
  SOURCE_CRC_ERROR        = 5,
  SOURCE_UNKNOWN          = 6
};

enum DMA_TRANSFER_MODE : int16_t {
  BURST_MODE = 0,
  BLOCK_MODE = 1
};

enum DMA_PRIORITY_LEVEL : int16_t {
  PRIORITY_LOW = 0,
  PRIORITY_MEDIUM = 1,
  PRIORITY_HIGH = 2,
  PRIORITY_HIGHEST = 3,
};

enum DMA_INCREMENT_MODIFIER : int16_t {
  x1   = 0,
  x2   = 1,
  x4   = 2,
  x8   = 3,
  x16  = 4,
  x32  = 5,
  x64  = 6,
  x128 = 7 
};

enum DMA_TARGET : int16_t {
  DESTINATION = 0,
  SOURCE = 1
};

enum DMA_TRIGGER_SOURCE : int16_t {
  TRIGGER_NONE              = 0,
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


///////////////////////////////////////////////////////////////////////////////////