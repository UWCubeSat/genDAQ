
#pragma once
#include <Arduino.h>
#include <GlobalDefs.h>

class DMAChannelSettings;
class DMA_Utility;
class DMAChannel;

void DMAC_0_Handler(void);

///////////////////////////////////////////////////////////////////////////////////

struct DMAChannelTask {
  friend DMAChannel;

  protected:
    int16_t taskID; 
    __attribute__((__aligned__(16))) DmacDescriptor taskDescriptor;

  public:
    // TO DO.

};

///////////////////////////////////////////////////////////////////////////////////

class DMAChannelSettings {
  friend DMAChannel;

  protected:
    void (*callbackFunction)(DMA_INTERRUPT_SOURCE, DMAChannel*) = nullptr;
    uint32_t CHCTRLA_mask = 0;
    int16_t priorityLevel = 0;
    int16_t defaultCycles = 0;
    // DOES NOT INCLUDE -> TRIGGER SOURCE (EXTERNAL)

  public:
    // TO DO.
    
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

    /*


    void getSystemSettings();

    void changeSystemSettings();
    */

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
    int16_t externalTriggerCycles = 0;
    void (*callbackFunction)(DMA_INTERRUPT_SOURCE, DMAChannel*) = nullptr;

    volatile bool initialized = false; 
    volatile bool paused = false;           
    volatile int16_t remainingCycles = 0;
    volatile DMA_CHANNEL_ERROR channelError = CHANNEL_ERROR_NONE;

  public:
    DMAChannel(const int16_t channelNumber);
    
    bool init(DMAChannelSettings &settings);

    bool exit();

    bool updateSettings(DMAChannelSettings &Settings); // NOT DONE

    bool setTasks(DMAChannelTask **tasks, int16_t numTasks);
    bool setTask(DMAChannelTask *task) { return setTasks(&task, 1); }

    bool addTask(DMAChannelTask *task); // NOT DONE

    bool removeTask(int16_t taskIndex);

    bool clearTasks();

    int16_t getTaskCount();

    bool start(int16_t cycles = -1);

    bool stop();

    bool pause();

    bool enableExternalTrigger(DMA_TRIGGER_SOURCE source, DMA_TRIGGER_ACTION action, int16_t cycles);

    bool disableExternalTrigger();

    bool changeExternalTrigger(DMA_TRIGGER_SOURCE newSource, DMA_TRIGGER_ACTION newAction, int16_t cycles);

    DMA_CHANNEL_STATUS getStatus();
    bool isBusy() { return getStatus() == 4; } 

    DMA_CHANNEL_ERROR getError();

  protected:

    void resetFields();

    DmacDescriptor *createDescriptor(const DmacDescriptor *referenceDescriptor);

};