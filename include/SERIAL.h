///////////////////////////////////////////////////////////////////////////////////////////////////
///// FILE -> PERIPHERALS
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <Arduino.h>
#include <SERCOM.h>
#include <wiring_private.h>

#include <GlobalTools.h>
#include <GlobalDefs.h>
#include <DMA.h>

class SIOBus;
class I2CBus;
class SPIBus;
class UARTBus;

class ADCModule;

void SercomIRQHandler();

typedef void (*SIOCallback)(int16_t sourceSercomID, int16_t param1, int16_t param2);

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> SIOBus BASE CLASS
///////////////////////////////////////////////////////////////////////////////////////////////////

class SIOBus {
  public:

    int16_t getSIOid();
    
    SIO_TYPE getType();

  protected:
    int16_t SIOid;
    SIO_TYPE baseType = TYPE_UNKNOWN;
    bool enabled;

    SIOBus() {}

    Sercom *allocSercom();

    void freeSercom(Sercom *freedInstance);

    void freePin(int16_t pinNum);

    static Sercom sercomPeriphs[SIO_MAX_SERCOM]; 
    static bool ports[SIO_MAX_PORTS];
};

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> I2C SERIAL CLASS
///////////////////////////////////////////////////////////////////////////////////////////////////

class I2CBus : public SIOBus {
  public:
    I2CBus(Sercom *s, uint8_t SDA, uint8_t SCL);

    ~I2CBus() { exit(); }

    bool requestData(uint8_t deviceAddr, uint16_t registerAddr, bool reg16);
    
    bool readData(int16_t bytes, void *datadestinationAddr);

    bool writeData(uint8_t deviceAddr, uint16_t registerAddr, bool reg16, 
      int16_t writeCount, void *dataSourceAddr);

    bool resetBus(bool hardReset);

    bool dataReady();

    bool isBusy();

    ERROR_ID getError();

    struct I2CSettings { 

        void setDefault();

        I2CSettings &setBaudrate(uint32_t buadrate);

        I2CSettings &setCallback(SIOCallback *callbackFunction);

        I2CSettings &setCallbackConfig(bool errorCallback, bool requestCompleteCallback,
          bool readCompleteCallback, bool writeCompleteCallback);

        I2CSettings &setSCLTimeoutConfig(bool enabled);

        I2CSettings &setInactiveTimeout(int16_t timeoutConfig);

        I2CSettings &setTransferSpeed(int16_t transferSpeedConfig);

        I2CSettings &setSCLClientTimeoutConfig(bool enabled);

        I2CSettings &setSCLHostTimeoutConfig(bool enabled);

        I2CSettings &setSDAHoldTime(int16_t sclHoldConfig);

        I2CSettings &setSleepConfig(bool enableDurringSleep);

      protected:
        friend I2CBus;
        I2CBus *super;
        explicit I2CSettings(I2CBus *super);
        ERROR_ID settingsError;

         inline I2CSettings &changeCTRLA(const uint32_t clearMask, const uint32_t setMask);

    } settings{this};

  protected:

    bool init();

    bool initDMA();

    void exit();

    void resetFields();

    void updateReg(int16_t regAddr, bool reg16);

  private:
    friend void I2CdmaCallback(DMA_CALLBACK_REASON reason, TransferChannel &channel, 
      int16_t triggerType, int16_t descIndex, ERROR_ID error);
    friend I2CSettings;

    //// PROPERTIES ////
    const uint8_t SDA;
    const uint8_t SCL;
    int16_t sNum;
    Sercom *s;                      // Dont Delete
    TransferChannel *readChannel;   // Dealloc using DMAUtil
    TransferChannel *writeChannel;  // Dealloc using DMAUtil
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
    SIOCallback *callback;
    int32_t baudrate;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> SPI SERIAL CLASS
///////////////////////////////////////////////////////////////////////////////////////////////////

class SPIBus : public SIOBus {
  public:

    SPIBus(Sercom *s, int16_t SCK, int16_t PICO, int16_t POCI, int16_t CS);

    bool readData(int16_t byteCount, uint32_t dataDestinationAddr);

  struct SPISettings {

    SPISettings &setClockPhaseConfig(int16_t clockPhaseConfig);

    SPISettings &setSleepConfig(bool enabledDurringSleep);

    SPISettings &setDataSize(int16_t dataSizeConfig);

    SPISettings &setPreloadConfig(bool enabled);

    void setDefault();

    private:
      friend SPIBus;
      SPIBus *super;
      explicit SPISettings(SPIBus *super);

      SPISettings & changeCTRLA(const uint32_t resetMask, const uint32_t setMask);

      SPISettings &changeCTRLB(const uint32_t resetMask, const uint32_t setMask);
  }settings{this};


  private:
    friend SPISettings;

    int16_t SCK;
    int16_t PICO;
    int16_t POCI;
    int16_t CS; 
    Sercom *s;
    int16_t sNum;

    void init();

    void initDMA();

};

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> UART SERIAL CLASS
///////////////////////////////////////////////////////////////////////////////////////////////////

class UARTBus : public SIOBus {

  public:

    UARTBus(Sercom *s) {}

    bool begin();

    bool end();

    bool attachPins(uint8_t rxPin, uint8_t txPin, int16_t rtsPin, int16_t ctsPin,
      uint8_t rxPad);

    bool detachPins();

    bool enable();

    bool read(void *dataDestination, uint16_t maxBytes);

    bool write(void *dataSource, uint16_t bytes);

    bool readBusy();

    bool writeBusy();
    
    bool abortOpperation();

    bool disable();

    struct UARTSettings {


      private:
        friend UARTBus;
        UARTBus *super;
        explicit UARTSettings();
    };


  private:
    friend UARTSettings;
    Sercom *s;
    uint8_t sercomNum;
    uint8_t txPin;
    uint8_t rxPin;
    int16_t rtsPin;
    int16_t ctsPin;
    uint8_t rxPad;

    bool begun;

    int16_t priorityLvl;

    void resetFields();

    void initPins();

    void exitPins();

};


