///////////////////////////////////////////////////////////////////////////////////////////////////
///// FILE -> PERIPHERALS
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <Arduino.h>
#include <SERCOM.h>
#include <wiring_private.h>

#include <GlobalTools.h>
#include <GlobalDefs.h>
#include <DMAUtility.h>

class IO;
class I2CSerial;
class SPIBus;
class UARTBus;
class AnalogPin;
class DigitalPin;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> PERIPHERAL MANAGER CLASS (SINGLETON)
///////////////////////////////////////////////////////////////////////////////////////////////////

class IOManager_ {
  friend I2CSerial;

  public:

    IO *getActiveIO(int16_t IOID); 

  private:
    IOManager_() {}
    static uint32_t referenceMillis;
    IO *activeIO[IO_MAX_INSTANCES];

    void abort(I2CSerial *instance);
    void abort(SPIBus *instance);

};
extern IOManager_ &IOManager;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> SERIAL I2C PERIPHERAL
///////////////////////////////////////////////////////////////////////////////////////////////////

class IO {
  public:
    IO() {}

    int16_t getBaseIOID();
    
    IO_TYPE getBaseType();

  protected:
    int16_t IOID;
    IO_TYPE baseType = TYPE_UNKNOWN;
};

class I2CSerial : public IO {
  public:
    I2CSerial(int16_t sercomNum, uint8_t SDA, uint8_t SCL, int16_t IOID);

    ~I2CSerial(); // TO DO

    bool requestRegister(uint8_t deviceAddr, uint16_t registerAddr, bool reg16);
    
    bool readData(int16_t readCount, void *dataDestination);

    bool writeData(uint8_t deviceAddr, uint16_t registerAddr, bool reg16, uint8_t *writeData, 
      int16_t writeCount, void *sourceArray);

    bool requestReady();

    bool readReady();

    bool writeReady();

    class I2CSettings { // TO DO
      public:

        void setDefault();

        bool setBaudrate(int32_t buadrate);

      protected:
        friend I2CSerial;
        I2CSerial *super;
        explicit I2CSettings(I2CSerial *super);

    } settings{this};

  protected:

    bool init();

    bool initDMA();

    void exit();

    void resetFields();

    I2C_STATUS updateStatus();

  private:
    //// PROPERTIES ////
    const uint8_t SDA;
    const uint8_t SCL;
    int16_t sercomNum;
    Sercom *s;                      // Dont Delete
    DMAChannel *readChannel;        // Dealloc using DMAUtil
    DMAChannel *writeChannel;       // Dealloc using DMAUtil
    TransferDescriptor *readDesc;   // Delete
    TransferDescriptor *writeDesc;  // Delete
    bool reg16;

    //// CACHE //// -> To be accessed by DMA
    uint8_t registerAddr[2];
    uint8_t deviceAddr;

    //// FLAGS ////
    int16_t lastRequest;
    I2C_STATUS currentStatus; 
    bool currentError;
    volatile bool transferComplete;

    //// SETTINGS ////
    I2CCallbackFunction *callback;
    int32_t baudrate;
};












class SPIBus : public IO {



};


class SerialBus : public IO {



};

