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

typedef void (*IOCallback)(int16_t sourceIOID, int16_t param1, int16_t param2);

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

    ~I2CSerial() { exit(); }

    bool requestData(uint8_t deviceAddr, uint16_t registerAddr, bool reg16);
    
    bool readData(int16_t bytes, void *datadestinationAddr);

    bool writeData(uint8_t deviceAddr, uint16_t registerAddr, bool reg16, 
      int16_t writeCount, void *dataSourceAddr);

    bool resetBus(bool hardReset);

    bool dataReady();

    bool isBusy();

    ERROR_ID getError(); /////////// TO DO

    struct I2CSettings { 

        void setDefault();

        I2CSettings &setBaudrate(uint32_t buadrate);

        I2CSettings &setCallback(IOCallback *callbackFunction);

        I2CSettings &setCallbackConfig(bool errorCallback, bool requestCompleteCallback,
          bool readCompleteCallback, bool writeCompleteCallback);

        I2CSettings &setSCLTimeoutConfig(bool enabled);

        I2CSettings &setInactiveTimeout(int16_t timeoutConfig);

        I2CSettings &setTransferSpeed(int16_t transferSpeedConfig);

        I2CSettings &setSDAHoldTime(int16_t sclHoldConfig);

        I2CSettings &setSleepConfig(int16_t sleepConfig);

      protected:
        friend I2CSerial;
        I2CSerial *super;
        explicit I2CSettings(I2CSerial *super);
        ERROR_ID settingsError;

    } settings{this};

  protected:

    bool init();

    bool initDMA();

    void exit();

    void resetFields();

    void updateReg(int16_t regAddr, bool reg16);

  private:
    friend void I2CdmaCallback(DMA_CALLBACK_REASON reason, TransferChannel &channel, 
      int16_t triggerType, int16_t descIndex, DMA_ERROR error);
    friend I2CSettings;

    //// PROPERTIES ////
    const uint8_t SDA;
    const uint8_t SCL;
    int16_t sercomNum;
    Sercom *s;                      // Dont Delete
    TransferChannel *readChannel;        // Dealloc using DMAUtil
    TransferChannel *writeChannel;       // Dealloc using DMAUtil
    TransferDescriptor *readDesc;   // Delete
    TransferDescriptor *writeDesc;  // Delete
    TransferDescriptor *regDesc;    // Delete

    //// CACHE //// -> To be accessed by DMA
    uint8_t registerAddr[2];
    uint8_t deviceAddr;

    //// FLAGS ////
    volatile bool criticalError; // Set by I2C interrupt when error detected
    volatile uint8_t busyOpp;    // Reset by DMA interrupt when transfer complete
    bool errorCallback,
         requestCompleteCallback,
         readCompleteCallback,
         writeCompleteCallback;

    //// SETTINGS ////
    IOCallback *callback;
    int32_t baudrate;
};



class SPIBus : public IO {



};


class UARTBus : public IO {



};


class AnalogPin : public IO {


};


class digitalPin : public IO {


};

