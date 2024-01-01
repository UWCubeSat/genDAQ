
#pragma once
#include <Arduino.h>
#include <GlobalDefs.h>

class DMAChannelSettings;
class DMA_Utility;
class DMAChannel;

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

  private:
    volatile bool initialized = false; // For getStatus();
    volatile bool paused = false;

    volatile bool pauseFlag = false;
    volatile bool swTriggerFlag = false;
    volatile bool activeFlag = false;

    const int16_t channelIndex = 0;
    int16_t currentDescriptorCount = 0;
    int16_t remainingCycles = 0;
    int16_t defaultCycles = 0;
    int16_t externalTriggerCycles = 0;
    volatile DMA_CHANNEL_ERROR channelError = CHANNEL_ERROR_NONE;
    void (*callbackFunction)(DMA_INTERRUPT_SOURCE, DMAChannel*);

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

    bool start(int16_t cycles = -1);

    bool stop();

    bool pause();

    bool setExternalTrigger(DMA_TRIGGER_SOURCE source, DMA_TRIGGER_ACTION action);

    bool removeExternalTrigger();

    DMA_CHANNEL_STATUS getStatus();
    bool isBusy() { return getStatus() == 4; } 

    DMA_CHANNEL_ERROR getError();


  protected:

    void resetFields();

    DmacDescriptor *createDescriptor(const DmacDescriptor *referenceDescriptor);

};