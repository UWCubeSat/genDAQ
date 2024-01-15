
#pragma once
#include <Arduino.h>
#include <GlobalDefs.h>

class DMAUtility;
class TransferChannel;
class ChecksumGen;
class TransferDescriptor;

typedef void (*DMACallbackFunction)(DMA_CALLBACK_REASON reason, TransferChannel &source, 
int16_t descriptorIndex, int16_t currentTrigger, ERROR_ID error);

typedef void (*ChecksumCallback)(ERROR_ID error, int16_t bytesWritten);

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> DMA UTILITY
///////////////////////////////////////////////////////////////////////////////////////////////////

class DMAUtility {
  private:
    DMAUtility() {}
    static TransferChannel channelArray[DMA_MAX_CHANNELS];
    static bool begun;
    static int16_t currentChannel;

  public:

      void begin();

      void end();

      TransferChannel &getChannel(int16_t channelIndex);

      TransferChannel &operator [] (int16_t channelIndex);

      TransferChannel *allocateChannel();
      TransferChannel *allocateChannel(int16_t ownerID);

      void freeChannel(int16_t channelIndex);
      void freeChannel(TransferChannel *channel);

      void resetChannel(int16_t channelIndex);
      void resetChannel(TransferChannel *channel);
};
extern DMAUtility &DMA;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> TRANSFER DESCRIPTOR
///////////////////////////////////////////////////////////////////////////////////////////////////

class TransferDescriptor {
  private:
    friend TransferChannel;
    friend ChecksumGen;
    DmacDescriptor desc;
    DmacDescriptor *currentDesc;
    uint32_t baseSourceAddr;
    uint32_t baseDestAddr;
    bool primaryBound;

  public:
    TransferDescriptor(void *source, void *destination, uint8_t transferAmountBytes);
    TransferDescriptor();
    TransferDescriptor(TransferDescriptor &other);

    TransferDescriptor &setSource(uint32_t sourceAddr, bool correctAddress);  // TO DO -> ADD setSource & setDestination for volatile void ptr
    TransferDescriptor &setSource(void *sourcePtr, bool correctAddress);

    TransferDescriptor &setDestination(uint32_t destinationAddr, bool correctAddress);
    TransferDescriptor &setDestination(void *destinationPtr, bool correctAddress);

    TransferDescriptor &setTransferAmount(uint16_t byteCount);

    TransferDescriptor &setDataSize(int16_t byteCount);

    TransferDescriptor &setIncrementConfig(bool incrementSource, bool incrementDestination);

    TransferDescriptor &setIncrementModifier(DMA_TARGET target, int16_t incrementModifier);

    TransferDescriptor &setAction(DMA_TRANSFER_ACTION action);

    void setDefault();

    bool isBindable();

    bool isValid();

    bool getError(); // TO DO

  private:

    DmacDescriptor *bindLink();

    void bindPrimary(DmacDescriptor *primaryDescriptor);

    void unbindPrimary(DmacDescriptor *primaryDescriptor);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> DMA CHANNEL
///////////////////////////////////////////////////////////////////////////////////////////////////

class TransferChannel {
  public:
    const int16_t channelIndex;

    bool setDescriptors(TransferDescriptor **descriptorArray, int16_t count, 
      bool bindDescriptors, bool udpateWriteback); 

    bool setDescriptor(TransferDescriptor *descriptor, bool bindDescriptor);

    bool replaceDescriptor(TransferDescriptor *updatedDescriptor, int16_t descriptorIndex,
      bool bindDescriptor);

    bool addDescriptor(TransferDescriptor *descriptor, int16_t descriptorIndex,
      bool bindDescriptor, bool updateWriteback);

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

    int16_t setOwnerID(int16_t newID);

    ERROR_ID getError();

    uint8_t getChannelNum();

    struct TransferSettings {
      
        TransferSettings &setTransferThreshold(int16_t elements);

        TransferSettings &setBurstLength(int16_t elements);

        TransferSettings &setTriggerAction(DMA_TRIGGER_ACTION action);

        TransferSettings &setStandbyConfig(bool enabledDurringStandby);

        TransferSettings &setPriorityLevel(int16_t priorityLevel);

        TransferSettings &setCallbackFunction(DMACallbackFunction callback);

        TransferSettings &setCallbackConfig(bool errorCallbacks, bool transferCompleteCallbacks,
          bool suspendCallbacks);

        TransferSettings &setDescriptorsLooped(bool descriptorsLooped, bool updateWriteback);
        
        void removeCallbackFunction();

        TransferSettings &setExternalTrigger(DMA_TRIGGER trigger);

        void removeExternalTrigger();

        void setDefault();

        void equals(TransferChannel &other);

      private:
        friend TransferChannel;
        TransferChannel *super;  
        explicit TransferSettings(TransferChannel *super);

    }settings{this};

    private:

      friend void DMAC_0_Handler(void);
      friend ChecksumGen;
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
      volatile ERROR_ID currentError;
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
      bool errorCallbacks;
      bool transferCompleteCallbacks;
      bool suspendCallbacks;

      TransferChannel(int16_t channelIndex);

    protected:

      DmacDescriptor *getDescriptor(int16_t descriptorIndex);

      void clear();

      void init(int16_t ownerID);

      void loopDescriptors(bool updateWriteback);

      void  unloopDescriptors(bool updateWriteback);
    
      void clearDescriptors();
};

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> CHECKSUM CHANNEL
///////////////////////////////////////////////////////////////////////////////////////////////////

class ChecksumGen {
  public:
    const int16_t ownerID;

    ChecksumGen(int16_t ownerID);

    bool start(int16_t checksumLength);

    bool stop(bool hardStop);

    bool isBusy();

    int16_t remainingBytes();

    ERROR_ID getError();

    struct ChecksumSettings {

      ChecksumSettings &setSource(uint32_t sourceAddress, bool correctAddress);
      ChecksumSettings &setSource(void *sourcePtr, bool correctAddress);

      ChecksumSettings &setDestination(uint32_t sourceAddress, bool correctAddress);
      ChecksumSettings &setDestination(void *destinationPtr, bool correctAddress);

      ChecksumSettings &setCRC(CRC_MODE mode);

      ChecksumSettings &setStandbyConfig(bool enabledDurringStandby);

      ChecksumSettings &setCallbackFunction(ChecksumCallback *callbackFunc);

      ChecksumSettings &setPriorityLevel(int16_t priorityLvl);

      void setDefault();

      private:
        friend ChecksumGen;
        ChecksumGen *super;
        explicit ChecksumSettings(ChecksumGen *super);

    }settings{this};

    ~ChecksumGen();

  private:
    friend ChecksumSettings;
    friend void ChecksumIRQHandler(DMA_CALLBACK_REASON reason, TransferChannel &source, 
      int16_t descriptorIndex, int16_t currentTrigger, ERROR_ID error);

    TransferChannel *channel;
    TransferDescriptor writeDesc;
    TransferDescriptor readDesc;
    ChecksumCallback *callback; 
    int16_t uniqueID;
  
  protected:
    bool descValid;
    volatile int16_t remainingCycles;
    bool mode32;

    void init();
};
