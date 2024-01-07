
#pragma once
#include <Arduino.h>
#include <GlobalDefs.h>

class DMAUtility;
class DMAChannel;
struct Descriptor;

typedef void (*DMACallbackFunction)(DMAChannel &channel, int16_t descriptorIndex, DMA_ERROR error);


class DMAUtility {
  private:
    static DMAChannel channelArray[DMA_MAX_CHANNELS];

  public:

    void begin();

    void end();

    DMAChannel &getChannel(int16_t channelIndex);


  private:
    DMAUtility() {}
    
};
extern DMAUtility &DMA;




struct Descriptor {
  DmacDescriptor desc;
  bool vaildDescriptor;

  public:
    Descriptor();

    Descriptor &setSource(void *source);

    Descriptor &setDestination(void *destination);

    Descriptor &setTransferAmount(uint16_t byteCount);

    Descriptor &setDataSize(int16_t byteCount);

    Descriptor &setIncrementConfig(bool incrementSource, bool incrementDestination);

    Descriptor &setIncrementModifier(DMA_TARGET target, int16_t incrementModifier);

    Descriptor &setAction(DMA_ACTION action);

    void setDefault();
};



class DMAChannel {
  private:
    friend void DMAC_0_Handler(void);

    const int16_t channelIndex;
    int16_t descriptorCount;

    DMA_TRIGGER externalTrigger;
    DMACallbackFunction callback;
    DMA_ERROR currentError;

    volatile bool swPendFlag;
    volatile bool swTriggerFlag;
    volatile bool transferErrorFlag;
    volatile bool suspendFlag;
    volatile int16_t currentDescriptor;

  public:

    bool setDescriptors(Descriptor **descriptorArray, int16_t count, bool loop);

    bool setDescriptor(Descriptor *descriptor, bool loop);

    bool trigger();

    bool suspend(bool blocking);

    bool resume();

    bool noaction();

    void disable(bool blocking);

    bool enable();

    bool enableTrigger();

    bool disableTrigger();

    bool getTriggerEnabled();

    void clear();

    bool isBusy();

    int16_t getCurrentDescriptor();

    int16_t getPending();

    bool isPending();

    DMA_STATUS getStatus();

    struct ChannelSettings {
      public:


      protected:
      friend DMAChannel;
      DMAChannel *super;  
      explicit ChannelSettings(DMAChannel *super);
    };

    private:

      DMAChannel(int16_t channelIndex);
    
      void clearDescriptors();


};