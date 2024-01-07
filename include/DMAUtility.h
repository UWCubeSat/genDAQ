
#pragma once
#include <Arduino.h>
#include <GlobalDefs.h>

class DMAUtility;
class DMAChannel;
struct TransferDescriptor;
struct DMAChannel::ChannelSettings;

typedef void (*DMACallbackFunction)(DMA_CALLBACK_REASON reason, DMAChannel &source, 
int16_t descriptorIndex, int16_t currentTrigger, DMA_ERROR error);

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> DMA UTILITY
///////////////////////////////////////////////////////////////////////////////////////////////////

class DMAUtility {
  private:
    DMAUtility() {}
    static DMAChannel channelArray[DMA_MAX_CHANNELS];
    static bool begun;
    static int16_t currentChannel;

  public:

    void begin();

    void end();

    DMAChannel &getChannel(int16_t channelIndex);

    DMAChannel &operator [] (int16_t channelIndex);

    DMAChannel *allocateChannel();
    DMAChannel *allocateChannel(int16_t ownerID);

    void freeChannel(int16_t channelIndex);

};
extern DMAUtility &DMA;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> DMA DESCRIPTOR
///////////////////////////////////////////////////////////////////////////////////////////////////

struct TransferDescriptor {
  DmacDescriptor desc;
  bool vaildDescriptor;

  public:
    TransferDescriptor();

    TransferDescriptor &setSource(void *source);

    TransferDescriptor &setDestination(void *destination);

    TransferDescriptor &setTransferAmount(uint16_t byteCount);

    TransferDescriptor &setDataSize(int16_t byteCount);

    TransferDescriptor &setIncrementConfig(bool incrementSource, bool incrementDestination);

    TransferDescriptor &setIncrementModifier(DMA_TARGET target, int16_t incrementModifier);

    TransferDescriptor &setAction(DMA_TRANSFER_ACTION action);

    void setDefault();
};

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> DMA CHANNEL
///////////////////////////////////////////////////////////////////////////////////////////////////

class DMAChannel {
  public:
    const int16_t channelIndex;

    bool setDescriptors(TransferDescriptor **descriptorArray, int16_t count, 
      bool loop, bool allocateDescriptor, bool preserveCurrentIndex);

    bool setDescriptor(TransferDescriptor *descriptor, bool loop);

    bool trigger();

    bool suspend(bool blocking);

    bool resume();

    bool noaction();

    void disable(bool blocking);

    bool enable();

    bool getEnabled();

    bool reset(bool blocking);

    bool queue(int16_t descriptorIndex);

    void enableTrigger();

    void disableTrigger();

    bool getTriggerEnabled();

    void clear();

    bool isBusy();

    int16_t getWritebackIndex();

    int16_t getPending();

    bool isPending();

    DMA_STATUS getStatus();

    int16_t getOwnerID();

    struct ChannelSettings {
      
        ChannelSettings &setTransferThreshold(int16_t elements);

        ChannelSettings &setBurstLength(int16_t elements);

        ChannelSettings &setTriggerAction(DMA_TRIGGER_ACTION action);

        ChannelSettings &setStandbyConfig(bool enabledDurringStandby);

        ChannelSettings &setPriorityLevel(int16_t priorityLevel);

        ChannelSettings &setCallbackFunction(DMACallbackFunction callback);
        
        void removeCallbackFunction();

        ChannelSettings &setExternalTrigger(DMA_TRIGGER trigger);

        void setDefault();

      private:
        friend DMAChannel;
        DMAChannel *super;  
        explicit ChannelSettings(DMAChannel *super);

    }settings{this};

    private:
      friend void DMAC_0_Handler(void);
      friend DMAUtility;

      //// GENERL FIELDS ////
      int16_t descriptorCount;
      bool descriptorsAllocated;

      //// DMA UTIL VALUES //// 
      bool allocated;
      int16_t ownerID;

      //// FLAGS ////
      bool externalTriggerEnabled;
      volatile DMA_ERROR currentError;
      volatile bool swPendFlag;
      volatile bool swTriggerFlag;
      volatile bool transferErrorFlag;
      volatile bool suspendFlag;
      volatile int16_t currentDescriptor;

      ///// SETTINGS ////
      DMA_TRIGGER externalTrigger;
      DMACallbackFunction callback;


      DMAChannel(int16_t channelIndex);

      void init(int16_t ownerID);
    
      void clearDescriptors();
};