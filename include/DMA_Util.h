
#pragma once
#include <Arduino.h>
#include <GlobalDefs.h>

class DMAChannelSettings;
class DMA_Utility;
class DMAChannel;

void DMAC_0_Handler(void);

///////////////////////////////////////////////////////////////////////////////////

struct DMATask {
  friend DMAChannel;

  protected:
    __attribute__((__aligned__(16))) DmacDescriptor taskDescriptor;

  public:
    DMATask();

    DMATask &setIncrementConfig(bool incrementDestination, bool incrementSource);

    DMATask &setIncrementModifier(DMA_INCREMENT_MODIFIER modifier, DMA_TARGET target);

    DMATask &setElementSize(int16_t bytes);

    DMATask &setTransferAmount(int16_t elements);

    DMATask &setSource(void* sourcePointer);

    DMATask &setDestination(void* destinationPointer);

    void resetSettings();
};

///////////////////////////////////////////////////////////////////////////////////

class DMAChannelSettings {
  friend DMAChannel;

  protected:
    void (*callbackFunction)(DMA_INTERRUPT_REASON, DMAChannel&) = nullptr;
    uint32_t settingsMask = 0;
    int16_t priorityLevel = 0;

  public:
    DMAChannelSettings();

    DMAChannelSettings &setTransferThreshold(int16_t elements);

    DMAChannelSettings &setBurstSize(int16_t elements);

    DMAChannelSettings &setTransferMode(DMA_TRANSFER_MODE mode);

    DMAChannelSettings &setSleepConfig(bool enabledDurringSleep);

    DMAChannelSettings &setPriorityLevel(DMA_PRIORITY_LEVEL priorityLevel);

    DMAChannelSettings &setCallbackFunction(void (*callbackFunction)(DMA_INTERRUPT_REASON, DMAChannel&));

    void removeCallbackFunction();

    void resetSettings();    
};

///////////////////////////////////////////////////////////////////////////////////

class DMA_Utility {
  private:
    static bool begun;
    static int16_t channelCount;
    static DMAChannel channelArray[DMA_MAX_CHANNELS];

  public:
    bool begin();

    bool end();

    DMAChannel &getChannel(const int16_t channelIndex);

    DMAChannel &operator[](const int16_t channelIndex);

  protected:
    DMA_Utility() {}   
};
extern DMA_Utility &DMA;

///////////////////////////////////////////////////////////////////////////////////

class DMAChannel { 
  friend DMA_Utility;
  friend void DMAC_0_Handler(void);

  protected:
    const int16_t channelIndex = 0;
    int16_t externalActions = 0;
    void (*callbackFunction)(DMA_INTERRUPT_REASON, DMAChannel&) = nullptr;

    volatile bool initialized = false; 
    volatile int16_t remainingActions = 0;
    volatile DMA_CHANNEL_ERROR channelError = CHANNEL_ERROR_NONE;

  public:
    DMAChannel(const int16_t channelNumber);
    
    bool init(DMAChannelSettings &settings);

    bool exit();

    bool updateSettings(DMAChannelSettings &Settings); 

    bool setTasks(DMATask **tasks, int16_t numTasks);
    bool setTask(DMATask *task) { return setTasks(&task, 1); }

    bool addTask(DMATask *task, int16_t taskIndex); 

    bool removeTask(int16_t taskIndex);

    bool clearTasks();

    int16_t getTaskCount();

    bool start(int16_t actions = 0);

    bool stop();

    bool pause();

    bool enableExternalTrigger(DMA_TRIGGER_SOURCE source, int16_t actions = 0);

    bool disableExternalTrigger();

    bool changeExternalTrigger(DMA_TRIGGER_SOURCE newSource, int16_t actions = 0);

    DMA_CHANNEL_STATUS getStatus();
    bool isBusy() { return getStatus() == 4; } 

    DMA_CHANNEL_ERROR getError();

  protected:

    void resetFields();

    DmacDescriptor *createDescriptor(const DmacDescriptor *referenceDescriptor);

};