
#include <Arduino.h>

void setup() {
  Serial.begin(0);
  while(!Serial);

  USBDeviceClass &usbp = USBDevice;

  uint8_t data = 54;



  USB->DEVICE.DeviceEndpoint[3].EPSTATUSSET.bit.BK1RDY = 1;

  usbp.armSend(CDC_ENDPOINT_IN, &data, 1);

  if (USB->DEVICE.DeviceEndpoint[3].EPSTATUS.bit.BK1RDY) {
    // previous transfer is still not complete

    // convert the timeout from microseconds to a number of times through
    // the wait loop; it takes (roughly) 23 clock cycles per iteration.
    uint32_t timeout = microsecondsToClockCycles(70 * 1000) / 23;

    // Wait for (previous) transfer to complete
    // inspired by Paul Stoffregen's work on Teensy
    while (!USB->DEVICE.DeviceEndpoint[3].EPINTFLAG.bit.TRCPT1) {
      if (timeout-- == 0) {
        usbp.sendZlp(CDC_ENDPOINT_IN);
        break;
        //?
      }
    }
  }

  USB->DEVICE.DeviceEndpoint[3].EPSTATUSSET.bit.BK1RDY = 1;



  usbp.armSend(CDC_ENDPOINT_IN, &data, 1);


}

void loop() {


}



///////////////////////////////////////////////////////////////////////////////////////////////////
