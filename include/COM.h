
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

    bool send(void *source, uint8_t bytes, bool cacheData = true);

    int16_t bytesSent();

    int16_t bytesRemaining();

    bool abortSend(bool blocking);

    bool recieve(void *destination, int16_t bytes);

    int16_t available();

    bool busy();

    ERROR_ID getError();

    struct COMSettings {

      private:
        friend COM_;
        COM_ *super;
        explicit COMSettings(COM_ *super) { this->super = super; }

    }settings{this};

  protected:

    COM_();

    void resetFields();

    void ackEP(int16_t epNum);

    void hostReset();

  private:
    friend COMSettings;
    friend void COMHandler(void);
    UsbDeviceDescriptor *endp[COM_EP_COUNT];
    USBDeviceClass *usbp;

    ERROR_ID currentError;
    Timeout sendTO;
    bool begun;

    COMCallback *callback;
};

extern COM_ &COM;


