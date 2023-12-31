
#pragma once
#include <Arduino.h>
#include <GlobalDefs.h>

class DMAChannelSettings;
class DMA_Utility;
class DMA_Channel;

///////////////////////////////////////////////////////////////////////////////////

class DMA_Utility {
  private:
    static bool begun;
    static int16_t channelCount;
    static DMA_Channel channelArray[DMA_MAX_CHANNELS];

  public:
    

    void begin();

    void end();

    DMA_Channel &getChannel(uint8_t channelNumber);
    /*
    void end();

    void getSystemSettings();

    void changeSystemSettings();
    */

  protected:
    DMA_Utility() {}   
};
extern DMA_Utility &DMA;

///////////////////////////////////////////////////////////////////////////////////

struct DMAChannelTask {
  friend DMA_Channel;

  protected:
    uint8_t taskNumber;
    uint8_t stepSize;
    uint8_t stepSelection;
    uint8_t destinationIncrement;
    uint8_t sourceIncrement;
    uint8_t beatSize;
    uint8_t blockTransferCount;
    uint8_t transferSourceAddress;
    uint8_t transferDestinationAddress;    

  public:
    // TO DO.

};

///////////////////////////////////////////////////////////////////////////////////

class DMAChannelSettings {
  friend DMA_Channel;

  protected:
    void (*callbackFunction)(DMA_Channel*);
    uint8_t beatSize;
    uint8_t burstSize;
    uint8_t transferThreshold;
    uint8_t triggerAction;
    uint8_t triggerSource;
    uint8_t runStandby;
    uint8_t priorityLevel;

  public:
    // TO DO.
    
};

///////////////////////////////////////////////////////////////////////////////////

class DMA_Channel { 
  friend DMA_Utility;

  private:
    const uint8_t channelNumber;
    bool initialized = false;
    volatile DMA_CHANNEL_STATUS channelStatus = DMA_STATUS_UNINITIALIZED;
    DMAChannelSettings *settings;
    DMAChannelTask *tasks[DMA_MAX_DESCRIPTORS];

  public:
    DMA_Channel(const uint8_t channelNumber);
    
    bool begin(DMAChannelSettings *settings);

    bool end();

    bool addTask(DMAChannelTask *channelTask);

    /*
    void removeTask();

    void clearTasks();

    void startTasks();

    void stopTasks();

    void getSettings();

    void changeSettings();

    void getStatus();

    void getErrors();
    */

  protected:

    void updateStatus();
};