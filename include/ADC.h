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

typedef void ADCWindowCallback(void);

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> ADC MODULE CLASS
///////////////////////////////////////////////////////////////////////////////////////////////////

class ADCModule {
  struct ADCSettings;
  friend ADCModule::ADCSettings;
  friend void ADCCommonHandler(ADCModule *source);
  friend void ctrlDMACallback (DMA_CALLBACK_REASON reason, TransferChannel &source, 
    int16_t descriptorIndex); 
  friend void dataDMACallback (DMA_CALLBACK_REASON reason, TransferChannel &source, 
    int16_t descriptorIndex); 

  public:
    const uint8_t moduleNumber;

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

      ADCSettings &setCustomDestination(void *destinationPtr);
      ADCSettings &setCustomDestination(uint32_t destinationAddr, bool isRegister);

      ADCSettings &removeCustomDestination();
      
      ADCSettings &setAutoStopConfig(bool enabled, uint16_t transferCount);

      ADCSettings &setPrescaler(uint8_t clockDivisor);

      ADCSettings &setSleepConfig(bool runWhileSleep);

      ADCSettings &setDifferentialMode(bool enableDifferentialMode);

      ADCSettings &setReferenceConfig(ADC_REFERENCE referenceConfig, bool enableReferenceBuffer);

      ADCSettings &setWindowModeConfig(ADC_WINDOW_MODE mode, uint16_t upperBound = 0,
        uint16_t lowerBound = 0, ADCWindowCallback *callback = nullptr, bool useAccumulatedResult);

      ADCSettings &setSampleCount(uint8_t sampleCount, uint8_t customDivisor = 0);

      ADCSettings &setResolution(uint8_t resolutionBits);

      ADCSettings &setSampleDuration(uint8_t clockCycles);

      ADCSettings &setGainCorrectionConfig(bool enableGainCorrection, uint16_t gainCorrectionValue);

      ADCSettings &setOffsetCorrectionConfig(bool enableOffsetCorrection, uint16_t offsetCorrectionValue);

      ADCSettings &setRail2RailConfig(bool enableRail2RailMode);

      ADCSettings &setPriorityLvl(uint8_t priorityLvl);

      ADCSettings &setDataTransferSize(uint16_t numBytes);

      ADCSettings &enableDigitalReferenceInput(uint8_t pinNumber, Dac *module,
         uint8_t referenceSelection); // Refers to the "REFSEL" register number for the DAC

      ADCSettings &disableDigitalReferenceInput();

      void setDefault();

    private:
      friend ADCModule;
      ADCModule *super;
      explicit ADCSettings(ADCModule *super);
      uint8_t idealResolution;
  }settings{this};

  protected:
    //// IMPORTANT ////
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
    int16_t DBLength;
    ADCWindowCallback *windowCB;
    int8_t erChannel; // External reference, disabled if negative
    uint8_t erType;
    Dac *erDAC;
    uint8_t dataResolution;
    bool resolutionSet;
    uint32_t cDest;
    bool cDestCorrect;
    uint16_t autoStopTC;
    bool autoStopEnabled;

    void resetFields();

    bool initDMA();

    void exitDMA(bool blocking);

    bool startDMA();

    bool setDescDefault();

    bool enableExternalRef();

    void disableExternalRef();
};


