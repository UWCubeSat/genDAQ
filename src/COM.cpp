
#include <COM.h>

void COM::begin() {

}

int16_t COM::send(void *source, uint8_t byteCount) {

  // Has previous message finished sending?
  if (USB->DEVICE.DeviceEndpoint[3].EPSTATUS.bit.BK1RDY
  || !USB->DEVICE.DeviceEndpoint[3].EPINTFLAG.bit.TRCPT1) {

    currentSendTO = millis() - startSendTO;
    if (currentSendTO >= timeout) {

      usbp.sendZlp(COM_EP_OUT);
      currentSendTO = 0;
      currentError = ERROR_COM_TIMEOUT;

    } else if (currentSendTO < 0) { // millis() overflows every ~70 min
      startSendTO = millis();
    }
    return 0;
  }

  // Handle exceptions
  if (byteCount > COM_SEND_MAX) {
    if (sendPartial) { byteCount = COM_SEND_MAX; }
    else             { return 0; }
  }

  if (cacheIndex + byteCount > maxCacheSize) return 0;

  //////////////// TO DO -> CACHE DATA HERE

  USB->DEVICE.DeviceEndpoint[COM_EP_OUT].EPSTATUSSET.bit.BK1RDY = 1; // "Ready" usb peripheral
  usbp.armSend(COM_EP_OUT, source, byteCount);                       // Input data into send buffer
  return byteCount;                     
}


int16_t COM::recieve(void *destination, uint8_t size) {
  usbp.recv(COM_EP_IN, destination, size);
} 

