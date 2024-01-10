
#pragma once
#include <Arduino.h>
#include <GlobalDefs.h>

class DMAUtility;
class DMAChannel;
class ChecksumChannel;
class TransferDescriptor;
struct DMAChannel::ChannelSettings;
struct ChecksumChannel::ChecksumSettings;

typedef void (*DMACallbackFunction)(DMA_CALLBACK_REASON reason, DMAChannel &source, 
int16_t descriptorIndex, int16_t currentTrigger, DMA_ERROR error);

typedef void (*ChecksumCallback)(DMA_ERROR error, int16_t bytesWritten);

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

    ChecksumChannel &getChecksumChannel();
  
    DMAChannel &operator [] (int16_t channelIndex);

    DMAChannel *allocateChannel();
    DMAChannel *allocateChannel(int16_t ownerID);

    void freeChannel(int16_t channelIndex);
    void freeChannel(DMAChannel *channel);

    void resetChannel(int16_t channelIndex);
    void resetChannel(DMAChannel *channel);
};
extern DMAUtility &DMA;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> TRANSFER DESCRIPTOR
///////////////////////////////////////////////////////////////////////////////////////////////////

class TransferDescriptor {
  private:
    friend DMAChannel;
    friend ChecksumChannel;
    DmacDescriptor desc;
    DmacDescriptor *currentDesc;
    uint32_t baseSourceAddr;
    uint32_t baseDestAddr;
    bool primaryBound;

  public:
    TransferDescriptor(void *source, void *destination, uint8_t transferAmountBytes);

    TransferDescriptor &setSource(uint32_t sourceAddr, bool correctAddress = true);
    TransferDescriptor &setSource(void *sourcePtr, bool correctAddress = true);

    TransferDescriptor &setDestination(uint32_t destinationAddr, bool correctAddress = true);
    TransferDescriptor &setDestination(void *destinationPtr, bool correctAddress = true);

    TransferDescriptor &setTransferAmount(uint16_t byteCount);

    TransferDescriptor &setDataSize(int16_t byteCount);

    TransferDescriptor &setIncrementConfig(bool incrementSource, bool incrementDestination);

    TransferDescriptor &setIncrementModifier(DMA_TARGET target, int16_t incrementModifier);

    TransferDescriptor &setAction(DMA_TRANSFER_ACTION action);

    void setDefault();

    bool isBindable();

    bool isValid();

  private:

    DmacDescriptor *bindLink();

    void bindPrimary(DmacDescriptor *primaryDescriptor);

    void unbindPrimary(DmacDescriptor *primaryDescriptor);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> DMA CHANNEL
///////////////////////////////////////////////////////////////////////////////////////////////////

class DMAChannel {
  public:
    const int16_t channelIndex;

    bool setDescriptors(TransferDescriptor **descriptorArray, int16_t count, 
      bool bindDescriptors = false, bool udpateWriteback = false); 

    bool setDescriptor(TransferDescriptor *descriptor, bool bindDescriptor = false);

    bool replaceDescriptor(TransferDescriptor *updatedDescriptor, int16_t descriptorIndex,
      bool bindDescriptor = false);

    bool addDescriptor(TransferDescriptor *descriptor, int16_t descriptorIndex,
      bool bindDescriptor = false, bool updateWriteback);

    bool removeDescriptor(int16_t descriptorIndex, bool updateWriteback); 

    bool trigger();

    bool suspend(bool blocking);

    bool resume();

    bool noaction();

    void disable(bool blocking);

    bool enable();

    bool getEnabled();

    bool resetTransfer(bool blocking);

    bool queue(int16_t descriptorIndex);

    bool enableExternalTrigger();

    bool disableExternalTrigger();

    bool getExternalTriggerEnabled();

    bool isBusy();

    bool setDescriptorValid(int16_t descriptorIndex, bool valid); // -1 targets wb

    bool setAllValid(bool valid);

    bool getDescriptorValid(int16_t descriptoerIndex);

    int16_t getLastIndex();

    int16_t remainingBytes();

    int16_t remainingBursts();

    int16_t getPending();

    bool isPending();

    bool syncBusy();

    DMA_STATUS getStatus();

    int16_t getOwnerID();

    struct ChannelSettings {
      
        ChannelSettings &setTransferThreshold(int16_t elements);

        ChannelSettings &setBurstLength(int16_t elements);

        ChannelSettings &setTriggerAction(DMA_TRIGGER_ACTION action);

        ChannelSettings &setStandbyConfig(bool enabledDurringStandby);

        ChannelSettings &setPriorityLevel(int16_t priorityLevel);

        ChannelSettings &setCallbackFunction(DMACallbackFunction callback);

        ChannelSettings &setDescriptorsLooped(bool descriptorsLooped, bool updateWriteback);
        
        void removeCallbackFunction();

        ChannelSettings &setExternalTrigger(DMA_TRIGGER trigger);

        void removeExternalTrigger();

        void setDefault();

      private:
        friend DMAChannel;
        DMAChannel *super;  
        explicit ChannelSettings(DMAChannel *super);

    }settings{this};

    private:
      friend void DMAC_0_Handler(void);
      friend ChecksumChannel;
      friend DMAUtility;

      //// GENERL FIELDS ////
      int16_t descriptorCount;
      TransferDescriptor *boundPrimary;
      int16_t previousIndex;              // For iteration optimization
      DmacDescriptor *previousDescriptor; // For iteration optimization

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
      uint8_t syncStatus;

      ///// SETTINGS ////
      DMA_TRIGGER externalTrigger;
      DMACallbackFunction callback;
      bool descriptorsLooped;

      DmacDescriptor *getDescriptor(int16_t descriptorIndex);

      DMAChannel(int16_t channelIndex);

      void clear();

      void init(int16_t ownerID);

      void loopDescriptors(bool updateWriteback);

      void  unloopDescriptors(bool updateWriteback);
    
      void clearDescriptors();

    protected:

      DMAChannel &base() { return *this; } // Allows derived classes to access overwritte methods in this class
};

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> CHECKSUM CHANNEL
///////////////////////////////////////////////////////////////////////////////////////////////////

class ChecksumChannel : private DMAChannel {
  public:

    bool start(int16_t checksumLength);

    bool stop(bool hardStop);

    bool isBusy();

    int16_t remainingBytes();

    DMA_ERROR getError();

    struct ChecksumSettings {

      ChecksumSettings &setSource(uint32_t sourceAddress, bool correctAddress = true);
      ChecksumSettings &setSource(void *sourcePtr, bool correctAddress = true);

      ChecksumSettings &setDestination(uint32_t sourceAddress, bool correctAddress = true);
      ChecksumSettings &setDestination(void *destinationPtr, bool correctAddress = true);

      ChecksumSettings &setCRC(CRC_MODE mode);

      ChecksumSettings &setStandbyConfig(bool enabledDurringStandby);

      ChecksumSettings &setCallbackFunction(ChecksumCallback *callbackFunc);

      ChecksumSettings &setPriorityLevel(int16_t priorityLvl);

      void setDefault();

      private:
        friend ChecksumChannel;
        ChecksumChannel *super;
        explicit ChecksumSettings(ChecksumChannel *super);
    }settings{this};

  private:
    friend DMAUtility;
    friend ChecksumSettings;
    friend void ChecksumIRQHandler(DMA_CALLBACK_REASON reason, DMAChannel &source, 
      int16_t descriptorIndex, int16_t currentTrigger, DMA_ERROR error);

    //// FIELDS ////
    TransferDescriptor writeDesc;
    TransferDescriptor readDesc;
    ChecksumCallback callback; 
    
    //// FLAGS ////
    bool descValid;
    volatile int16_t remainingCycles;

    //// SETTINGS ////
    bool mode32;


    ChecksumChannel(int16_t index);

    void init();
};
