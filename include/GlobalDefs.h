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

// Works with signed or unsigned numbers
#define SDIV_CEIL(x, y) (x / y + !(((x < 0) != (y < 0)) || !(x % y)))

// Works with ONLY POSITIVE numbers
#define UDIV_CEIL(x, y) (x / y + !!(x % y))

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> PROGRAM (GENERAL)
///////////////////////////////////////////////////////////////////////////////////////////////////

#define DEBUG_MODE true
#define BOARD_FEATHER_M4_EXPRESS_CAN__
#define BOARD_PIN_COUNT 44
#define BOARD_ADC_PERIPH 1

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> GLOBAL TOOLS
///////////////////////////////////////////////////////////////////////////////////////////////////

//// TIMEOUT CLASS ////
#define TO_DEFAULT_TIMEOUT 500

//// ERROR SYS ASSERTS ////
#define ERRSYS_ASSERTS_ENABLED true
#define ERRSYS_ASSERT_RESET_ENABLED true
#define ERRSYS_ASSERT_LED_ENABLED true
#define ERRSYS_ASSERT_LED_PULSES 50
#define ERRSYS_ASSERT_LED_DELAY_MS 50
#define ERRSYS_ASSERT_LED_PRIMARY 7
#define ERRSYS_ASSERT_LED_SECONDARY 13

//// ERROR SYS THROW ////
#define ERRSYS_ERROR_LED_ENABLED true
#define ERRSYS_ERROR_LED_DELAY_MS 100
#define ERRSYS_ERROR_LED 7

//// ERROR SYS MISC ////
#define ERRSYS_SERIAL_TIMEOUT 1000
#define ERRSYS_MAX_ERRORS 30

//// ENUMS ////
enum ERROR_ID : uint8_t {
  ERROR_NONE,
  ERROR_UNKNOWN,
  ERROR_NULLPTR,
  ERROR_SETTINGS_OOB,
  ERROR_SETTINGS_INVALID,
  ERROR_SETTINGS_OTHER,
  ERROR_DMA_TRANSFER,
  ERROR_DMA_CRC,
  ERROR_DMA_DESCRIPTOR,

  ERROR_COM_TIMEOUT,
  ERROR_COM_REQ,
  ERROR_COM_SYS,
  ERROR_COM_SEND,
  ERROR_COM_RECEIVE,
  ERROR_COM_MEM,

  ERROR_ADC_SYS,
  ERROR_ADC_DMA,
  ERROR_ADC_EXREF
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
#define DMA_MAX_CHECKSUM 2

//// DMA DEFAULT DESCRIPTOR SETTINGS ////
#define DMA_DEFAULT_DATA_SIZE DMAC_BTCTRL_BEATSIZE_BYTE_Val
#define DMA_DEFAULT_STEPSIZE DMAC_BTCTRL_STEPSIZE_X1_Val
#define DMA_DEFAULT_STEPSELECTION DMAC_BTCTRL_STEPSEL_SRC_Val
#define DMA_DEFAULT_TRANSFER_ACTION DMAC_BTCTRL_BLOCKACT_SUSPEND_Val
#define DMA_DEFAULT_INCREMENT_SOURCE 1
#define DMA_DEFAULT_INCREMENT_DESTINATION 1
#define DMA_DEFAULT_CRC_MODE 0
#define DMA_DEFAULT_CRC_LENGTH 16
#define DMA_DEFAULT_TRANSFER_AMOUNT 1

//// DMA DEFAULT CHANNEL SETTINGS ////
#define DMA_DEFAULT_TRANSMISSION_THRESHOLD 0
#define DMA_DEFAULT_BURST_LENGTH 0
#define DMA_DEFAULT_TRIGGER_ACTION 0
#define DMA_DEFAULT_TRIGGER_SOURCE TRIGGER_SOFTWARE
#define DMA_DEFAULT_RUN_STANDBY 1
#define DMA_DEFAULT_PRIORITY_LVL 1

//// CHECKSUM CHANNEL ////
#define CHECKSUM_MAX_TRANSFER_AMOUNT 4


//// CHECKSUM CHANNEL SETTINGS ////
#define CHECKSUM_DEFAULT_DATASIZE 1
#define CHECKSUM_DEFAULT_SLEEPCONFIG false
#define CHECKSUM_READDESC_DEFAULT_TRANSFER_ACTION ACTION_SUSPEND
#define CHECKSUM_WRITEDESC_DEFAULT_TRANSFER_ACTION ACTION_NONE
#define CHECKSUM_DEFAULT_TRANSFER_AMOUNT 4
#define CHECKSUM_READDESC_REQUIRED_SOURCE DMAC->CRCCHKSUM.reg
#define CHECKSUM_WRITEDESC_REQUIRED_DESTINATION DMAC->CRCDATAIN.reg
#define CHECKSUM_READDESC_REQUIRED_CORRECT_SOURCEADDR false
#define CHECKSUM_WRITEDESC_REQUIRED_CORRECT_DESTADDR false
#define CHECKSUM_REQUIRED_INCREMENT_CONFIG_SOURCE true
#define CHECKSUM_REQUIRED_INCREMENT_CONFIG_DESTINATION true
#define CHECKSUM_DEFAULT_CHECKSUM32 false

//// ENUMS ////
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
  ACTION_TRANSFER_BLOCK = 0,
  ACTION_TRANSFER_BURST = 2,
  ACTION_TRANSFER_ALL = 3
};
enum DMA_STATUS : uint8_t {
  DMA_CHANNEL_BUSY,
  DMA_CHANNEL_SUSPENDED,
  DMA_CHANNEL_ERROR,
  DMA_CHANNEL_IDLE,
  DMA_CHANNEL_DISABLED
};
enum DMA_CALLBACK_REASON : uint8_t {
  REASON_SUSPENDED,
  REASON_TRANSFER_COMPLETE_STOPPED,
  REASON_TRANSFER_COMPLETE_SUSPENDED,
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
enum CRC_MODE : uint8_t {
  CRC_16 = 0,
  CRC_32 = 1
};

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> SIO
///////////////////////////////////////////////////////////////////////////////////////////////////

#define SERCOM_IRQ_COUNT 4
#define SERCOM_FREQ_REF 48000000ul

struct SERCOM_REF_OBJ {
  uint8_t DMAReadTrigger; 
  uint8_t DMAWriteTrigger;
  uint8_t clock;
  IRQn_Type baseIRQ;
};


#define SIO_MAX_INSTANCES 10
#define SIO_MAX_SERCOM 5
#define SIO_MAX_PORTS 25

enum SIO_TYPE {
  TYPE_UNKNOWN,
  TYPE_I2CSERIAL,
  TYPE_SPISERIAL,
  TYPE_UARTSERIAL
};

/////////////////////////////////////////// I2C SERIAL ////////////////////////////////////////////

//// I2C SERIAL CLASS ////
#define I2C_IRQ_PRIORITY 1
#define I2C_MAX_READ 64
#define I2C_MAX_WRITE 32
#define I2C_STOP_CMD 3
#define I2C_BUS_IDLE_STATE 1
#define I2C_MAX_TRANSFERSPEED 2
#define I2C_MAX_SDAHOLDTIME 3

#define I2C_WRITE_TAG 200
#define I2C_READ_TAG 201

//// I2C SETTINGS ////
#define I2C_DEFAULT_BAUDRATE 100000
#define I2C_MAX_BAUDRATE 3400000
#define I2C_MIN_BAUDRATE 10000
#define I2C_MAX_SCLTIMEOUT_CONFIG 3


#define SPI_IRQ_COUNT 5


enum I2C_STATUS : uint8_t {
  I2C_IDLE,
  I2C_REQUEST_BUSY,
  I2C_REQUEST_COMPLETE,
  I2C_WRITE_BUSY,
  I2C_READ_BUSY,
  I2C_ERROR
};

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> ADC
///////////////////////////////////////////////////////////////////////////////////////////////////

#define ADC_MAX_PINS 12
#define ADC_IRQ_COUNT 2
#define ADC_MODULE_COUNT 2
#define ADC_MAX_MODULE_NUM 1
#define ADC_DEFAULT_MODULE ADC0
#define ADC_DEFAULT_MODULE_NUM 0

#define ADC_DB_LENGTH 512
#define ADC_DB_INCREMENT 124
#define ADC_DEFAULT_DB_OVERCLEAR 32

enum ADC_MODULE : uint8_t {
  ADC_MODULE0,
  ADC_MODULE1
};

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> COM CLASS
///////////////////////////////////////////////////////////////////////////////////////////////////

#define COM_PACKET_SIZE 64
#define COM_SEND_MAX_PACKETS 16
#define COM_RX_SIZE 512
#define COM_RX_PACKETS 8
#define COM_DEFAULT_REQ 1
#define COM_DEFAULT_RECIEVE 1

#define COM_EP_COUNT 4
#define COM_EP_ACM 1
#define COM_EP_IN 2
#define COM_EP_OUT 3
#define COM_EP_CTRL 0

#define COM_REASON_UNKNOWN 0
#define COM_REASON_RECEIVE_READY 1
#define COM_REASON_RECEIVE_FAIL 2
#define COM_REASON_SEND_COMPLETE 3
#define COM_REASON_SEND_FAIL 4
#define COM_REASON_RESET 5
#define COM_REASON_SOF 6

#define COM_MAX_SQ 3

#define COM_DEFAULT_RECEIVE_READY 1
#define COM_DEFAULT_RECEIVE_FAIL 1
#define COM_DEFAULT_SEND_COMPLETE 1
#define COM_DEFAULT_SEND_FAIL 1
#define COM_DEFAULT_RESET 0
#define COM_DEFAULT_SOF 0
#define COM_DEFAULT_SQ 0
#define COM_DEFAULT_TIMEOUT 500
#define COM_DEFAULT_ENFORCE_NUM 0
#define COM_DEFAULT_CALLBACK nullptr
#define COM_DEFAULT_CBRMASK (                                \
    (COM_DEFAULT_RECEIVE_READY << COM_REASON_RECEIVE_READY)  \
  | (COM_DEFAULT_RECEIVE_FAIL << COM_REASON_RECEIVE_FAIL)    \
  | (COM_DEFAULT_SEND_COMPLETE << COM_REASON_SEND_COMPLETE)  \
  | (COM_DEFAULT_SEND_FAIL << COM_REASON_SEND_FAIL)          \
  | (COM_DEFAULT_RESET << COM_REASON_RESET)                  \
  | (COM_DEFAULT_SOF << COM_REASON_SOF))



