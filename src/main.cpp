
#include <Arduino.h>

void setup() {
  Serial.begin(0);
  while(!Serial);

  Serial.println("test");

  

  
  Serial.println("test2");
/*
  USB->DEVICE.DeviceEndpoint[3].EPSTATUSSET.bit.BK1RDY = 1;
  usbp.armSend(CDC_ENDPOINT_IN, &data, 1);
  if (USB->DEVICE.DeviceEndpoint[3].EPSTATUS.bit.BK1RDY) {
    uint32_t timeout = microsecondsToClockCycles(70 * 1000) / 23;
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
  */

}

void loop() {


}



///////////////////////////////////////////////////////////////////////////////////////////////////
