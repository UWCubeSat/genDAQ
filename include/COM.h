
#pragma once
#include<Arduino.h>
#include <GlobalDefs.h>

uint8_t outCache[COM_MAX_CACHE_SIZE] = { 0 };

// Bank 1 is used for arduino write implementation...
// Bank 0 is used for read implementation
class COM : public USBDeviceClass {
  public:

    void begin();

    int16_t send(void *source, uint8_t size);

    int16_t recieve(void *destination, uint8_t size);

    struct COMSettings {

      private:
        friend COM_;
        COM *super;
        explicit COMSettings(COM *super);

    }settings{this};

  private:
    friend COMSettings;
    USBDeviceClass &usbp = USBDevice;
    ERROR_ID currentError;
    int16_t cacheIndex;

    int32_t startSendTO;
    int16_t currentSendTO;
    int16_t timeout;

    bool sendPartial;
    int16_t maxCacheSize;

};
extern COM &COM;
