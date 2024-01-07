///////////////////////////////////////////////////////////////////////////////////////////////////
///// FILE -> PERIPHERALS
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <Arduino.h>
#include <SERCOM.h>
#include <wiring_private.h>

#include <GlobalUtils.h>
#include <GlobalDefs.h>

int32_t referenceMillis = 0;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> I2C SENSOR
///////////////////////////////////////////////////////////////////////////////////////////////////

inline int16_t getInstruction(const uint8_t *instructionArray, const uint8_t value, 
  const int16_t maxIndex);



class Peripheral {
};

class I2CBus : public Peripheral {
  public:
    I2CBus(Sercom *s, SERCOM *sercom, uint8_t SDA, uint8_t SCL);

     int16_t collectData(const uint32_t dataBufferAddr);

     class I2CSettings {
      public:

        void setDefault();

      protected:
        friend I2CBus;
        I2CBus *super;
        explicit I2CSettings(I2CBus *super);

    } settings{this};

  protected:

    void initI2C();

    struct I2CDevice {
      uint8_t addr;
      uint8_t regCount;
      uint8_t *regAddrArray;
      uint8_t *regCountArray;
    };

  private:
    //// FIELDS ////
    const uint8_t SDA;
    const uint8_t SCL;
    SERCOM *sercom;
    Sercom *s;

    I2CDevice *deviceArray;
    int16_t deviceCount;
    ERROR_ID currentError;

    int32_t baudrate;
    int16_t cycles;
};












class SPIBus : public Peripheral {



};


class SerialBus : public Peripheral {



};

