
#pragma once
#include<Arduino.h>
#include <GlobalDefs.h>
#include <GlobalTools.h>

typedef void (*COMCallback)(uint8_t callbackReason);

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> COM CLASS
///////////////////////////////////////////////////////////////////////////////////////////////////

// Bank 1 is used for arduino write implementation...
// Bank 0 is used for read implementation
class COM_ : public USBDeviceClass {
  public:

    bool begin(USBDeviceClass *usbp); 

    bool end(); 

    bool sendPackets(void *source, uint16_t numPackets); 

    bool sendBusy();

    int16_t packetsSent();

    int16_t packetsRemaining();

    bool abortSend();

    int16_t recievePackets(void *destination, uint16_t numPackets, bool forceRecieve); 

    uint8_t *inspectPacket(uint16_t packetIndex); 

    bool request(void *customDest, uint16_t packetNum, bool forceReq);

    bool flush(int16_t numPackets);

    int16_t available(bool packets);

    int16_t packetsRecieved(); 

    bool requestPending();

    void cancelRequest(); 

    int16_t getPeripheralState();

    int32_t getFrameCount(bool getMicroFrameCount);

    ERROR_ID getError();

    void clearError();

    struct COMSettings {

      COMSettings &setServiceQuality(int16_t serviceQualityLevel);

      COMSettings &setCallbackConfig(bool enableRecvRdy, bool enableRecvFail, 
        bool enableSendCompl, bool enableSendFail, bool enableRst, bool enableSOF);

      COMSettings &setCallback(COMCallback *callback);

      COMSettings &setTimeout(uint32_t timeout);

      COMSettings &setEnforceNumPacketConfig(bool enforceNumPackets);

      void setDefault();

      private:
        friend COM_;
        COM_ *super;
        explicit COMSettings(COM_ *super) { this->super = super; }

    }settings{this};

  protected:

    COM_();

    void resetFields();

    void initEP();

    void resetSize(int16_t endpoint);

  private:
    //// Fields ////
    friend COMSettings;
    friend void COMHandler(void);
    UsbDeviceDescriptor *endp[COM_EP_COUNT];
    USBDeviceClass *usbp;
    Timeout sendTO;
    Timeout otherTO;

    //// Variables ////
    uint8_t RX[COM_RX_SIZE];
    volatile int16_t RXi; // Packet index
    volatile int16_t rxiLast;
    volatile bool rxiActive;
    volatile ERROR_ID currentError;
    bool begun;
    
    //// Settings ////
    COMCallback *callback;
    bool enforceNumPackets;
    uint8_t cbrMask;
    uint32_t STOtime;
    uint32_t OTOtime;
};

extern COM_ &COM;


