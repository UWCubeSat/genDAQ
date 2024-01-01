
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

#define DMA_MAX_CHANNELS 10
#define DMA_MAX_TASKS 4
#define DMA_IRQ_COUNT 5
#define DMA_PRIORITY_LVL_COUNT 4
#define DMA_MAX_CYCLES 100


#define DMA_DEFAULT_TRIGGER_SOURCE 0
#define DMA_DEFAULT_TRANSMISSION_THRESHOLD 0
#define DMA_DEFAULT_TRANSMISSION_SIZE 0
#define DMA_DEFAULT_TRIGGER_SOURCE 0
#define DMA_DEFAULT_PRIORITY_LEVEL 0
#define DMA_DEFAULT_RUN_STANDBY true

#define DMA_REQUIRED_TRIGGER_ACTION 2

///////////////////////////////////////////////////////////////////////////////////

enum DMA_CHANNEL_STATUS : int16_t {
  CHANNEL_STATUS_NULL = 0,
  CHANNEL_STATUS_IDLE,
  CHANNEL_STATUS_PAUSED,
  CHANNEL_STATUS_BUSY
};

enum DMA_CHANNEL_ERROR : int16_t {
  CHANNEL_ERROR_NONE,
  CHANNEL_ERROR_DESCRIPTOR_ERROR,
  CHANNEL_ERROR_CRC_ERROR,
  CHANNEL_ERROR_TRANSFER_ERROR
};

enum DMA_INTERRUPT_SOURCE : int16_t {
  CHANNEL_SUSPEND,
  TRANSFER_COMPLETE,
  CHANNEL_ERROR
};

enum DMA_TRIGGER_SOURCE : int16_t {
  NONE = 0,
  RTC_TIMESTAMP = 1,
  DSU_DCC0 = 2,
  DSU_DCC1 = 3,
  SERCOM0_RX = 4,
  SERCOM0_TX = 5,
  SERCOM1_RX = 6,
  SERCOM1_TX = 7,
  SERCOM2_RX = 8,
  SERCOM2_TX = 9,
  SERCOM3_RX = 10,
  SERCOM3_TX = 11,
  SERCOM4_RX = 12,
  SERCOM4_TX = 13,
  SERCOM5_RX = 14,
  SERCOM5_TX = 15,
  SERCOM6_RX = 16,
  SERCOM6_TX = 17,
  SERCOM7_RX = 18,
  SERCOM7_TX = 19,
  CAN0_DEBUG_REQ = 20,
  CAN1_DEBUG_REQ = 21,
  TCC0_OOB = 22,
  TCC0_COMPARE_0 = 23,
  TCC0_COMPARE_1 = 24,
  TCC0_COMPARE_2 = 25,
  TCC0_COMPARE_3 = 26,
  TCC0_COMPARE_4 = 27,
  TCC0_COMPARE_5 = 28,
  TCC1_OOB = 29,
  TCC1_COMPARE_0 = 30,
  TCC1_COMPARE_1 = 31,
  TCC1_COMPARE_2 = 32,
  TCC1_COMPARE_3 = 33,
  TCC2_OOB = 34,
  TCC2_COMPARE_0 = 35,
  TCC2_COMPARE_1 = 36,
  TCC2_COMPARE_2 = 37,
  TCC3_OOB = 38,
  TCC3_COMPARE_0 = 39,
  TCC3_COMPARE_1 = 40,
  TCC4_OOB = 41,
  TCC4_COMPARE_0 = 42,
  TCC4_COMPARE_1 = 43,
  TC0_OOB = 44,
  TC0_COMPARE_0 = 45,
  TC0_COMPARE_1 = 46,
  TC1_OOB = 47,
  TC1_COMPARE_0 = 48,
  TC1_COMPARE_1 = 49,
  TC2_OOB = 50,
  TC2_COMPARE_0 = 51,
  TC2_COMPARE_1 = 52,
  TC3_OOB = 53,
  TC3_COMPARE_0 = 54,
  TC3_COMPARE_1 = 55,
  TC4_OOB = 56,
  TC4_COMPARE_0 = 57,
  TC4_COMPARE_1 = 58,
  TC5_OOB = 59,
  TC5_COMPARE_0 = 60,
  TC5_COMPARE_1 = 61,
  TC6_OOB = 62,
  TC6_COMPARE_0 = 63,
  TC6_COMPARE_1 = 64,
  TC7_OOB = 65,
  TC7_COMPARE_0 = 66,
  TC7_COMPARE_1 = 67,
  ADC0_RESRDY = 68,
  ADC0_SEQ = 69,
  ADC1_RESRDY = 70,
  ADC1_SEQ = 71,
  DAC_EMPTY0 = 72,
  DAC_EMPTY1 = 73,
  DAC_RESULT_READY0 = 74,
  DAC_RESULT_READY1 = 75,
  I2S_RX0 = 76,
  I2S_RX1 = 77,
  I2S_TX0 = 78,
  I2S_TX1 = 79,
  PCC_RX = 80,
  AES_WRITE = 81,
  AES_READ = 82,
  QSPI_RX = 83,
  QSPI_TX = 84
};

enum DMA_TRIGGER_ACTION : int16_t {
  TRANSFER_BLOCK,
  TRANSFER_BURST,
  TRANSFER_ALL
};

enum DMA_PRIORITY_LEVEL : int16_t {
  LOW_PRIORITY = 0,
  MEDIUM_PRIORITY = 1,
  HIGH_PRIORITY = 2,
  HIGHEST_PRIORITY = 3
};

///////////////////////////////////////////////////////////////////////////////////