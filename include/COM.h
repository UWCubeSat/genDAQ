
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

    bool begin(USBDeviceClass *usbp); // NEEDS WORK...

    bool end(); // TO DO

    bool restart(); // TO DO

    bool sendPackets(void *source, uint16_t numPackets); 

    bool sendBusy();

    int16_t packetsSent();

    int16_t packetsRemaining();

    bool abort();

    int16_t recievePackets(void *destination, uint16_t numPackets); 

    uint8_t *inspectPackets(uint16_t packetIndex); 

    void flush(); 

    int16_t available();

    int16_t recieved(); 

    bool queueDestination(void *destination);

    bool recievePending();

    ERROR_ID getError();

    void clearError();

    struct COMSettings {

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
    friend COMSettings;
    friend void COMHandler(void);
    UsbDeviceDescriptor *endp[COM_EP_COUNT];
    USBDeviceClass *usbp;
    Timeout sendTO;
    Timeout otherTO;

    uint8_t RX[COM_SEND_MAX_PACKETS];
    volatile int16_t RXi;
    volatile uint32_t customDest;
    volatile uint32_t queueDest;

    volatile ERROR_ID currentError;
    bool begun;
    
    COMCallback *callback;
    bool enforceNumPackets;
    uint16_t cbrMask;
    int16_t rxPacketCount;
};

extern COM_ &COM;


