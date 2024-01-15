///////////////////////////////////////////////////////////////////////////////////////////////////
///// FILE -> PIN IO
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <Arduino.h>
#include <GlobalTools.h>
#include <GlobalDefs.h>
#include <DMA.h>

class ADCModule;
struct ADCPeripheral;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> ADC MODULE CLASS
///////////////////////////////////////////////////////////////////////////////////////////////////

class ADCModule {

  public:

    ADCModule(uint8_t moduleNum);

    bool begin();

    void end(bool blocking); // TO DO

    bool addPin(uint8_t pinNum); 

    bool removePin(uint8_t pinNum);

    bool getPinValid(uint8_t pinNum);

    bool enable(); 

    void disable(); 

    void flushBuffer();

    bool syncBusy();

    ~ADCModule();

    struct ADCSettings {

      

      void setDefault();

    private:
      friend ADCModule;
      ADCModule *super;
      explicit ADCSettings(ADCModule *super);
  }settings{this};

  protected:
    //// IMPORTANT ////
    friend ADCSettings;
    friend void ADC0_0_Handler(void);
    int16_t adcNum;
    Adc *adc;
    TransferChannel *dataChannel;
    TransferChannel *ctrlChannel;
    uint8_t dataChNum;
    uint8_t ctrlChNum;
    TransferDescriptor dataDesc;
    TransferDescriptor ctrlDesc;
    uint32_t ctrlInput[ADC_MAX_PINS];
    uint16_t *DB;

    volatile int16_t ctrlIndex;
    volatile int16_t DBIndex;
    volatile int16_t currentState = 0;


    //// FIELDS ////
    int16_t pins[ADC_MAX_PINS];
    uint8_t pinCount;
    uint8_t activePins;
    ERROR_ID currentError;

    //// SETTINGS ////
    uint8_t priorityLvl;
    uint16_t dataTransferSize;
    int8_t erChannel; // External reference, disabled if negative
    uint8_t erType;
    Dac *erDAC;

    void resetFields();

    bool initDMA();

    void exitDMA(bool blocking);

    bool startDMA();

    bool setDescDefault();
};


