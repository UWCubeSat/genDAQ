
#include <COM.h>

COM_ &COM;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> COM INTERRUPT HANDLER
///////////////////////////////////////////////////////////////////////////////////////////////////

void COMHandler(void) {

  // Setup/reset req interrupt
  if (USB->DEVICE.DeviceEndpoint[0].EPINTFLAG.bit.RXSTP
  || USB->DEVICE.INTFLAG.bit.EORST) {                                
    COM.usbp->ISRHandler();
    COM.hostReset();        
  
  // Send complete interrupt
  } else if (USB->DEVICE.DeviceEndpoint[COM_EP_OUT].EPINTFLAG.bit.TRCPT1) {



    USB->DEVICE.DeviceEndpoint[COM_EP_OUT].EPSTATUSSET.bit.BK1RDY = 1; // Ready next transfer  
    USB->DEVICE.DeviceEndpoint[COM_EP_OUT].EPINTFLAG.bit.TRCPT1 = 1;   // Clear transfer complete flag

  } else {
    COM.usbp->ISRHandler();
  }

}

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> COM PUBLIC METHODS
///////////////////////////////////////////////////////////////////////////////////////////////////

bool COM_::begin(USBDeviceClass *usbp) {

  // Check exceptions
  if (!SERIAL_PORT_MONITOR) return false;

  resetFields();
  this->usbp = usbp;
  
  // Get endpoint descriptors
  for (int16_t i = 0; i < COM_EP_COUNT; i++) {
    endp[i] = (UsbDeviceDescriptor*)USB->DEVICE.DESCADD.bit.DESCADD 
      + (sizeof(UsbDeviceDescriptor) * i);

    if (endp[i] == nullptr) {
      currentError = ERROR_COM_SYS;
      return false;
    }
  }

  //USB_SetHandler(&COMHandler);
  //USB->DEVICE.DESCADD.bit.DESCADD = &epArray;

}

bool COM_::send(void *source, uint8_t bytes, bool cacheData = true) {

  // Has previous message finished sending?
  sendTO.start(false);
  if (USB->DEVICE.DeviceEndpoint[COM_EP_OUT].EPSTATUS.bit.BK1RDY
  || !USB->DEVICE.DeviceEndpoint[COM_EP_OUT].EPINTFLAG.bit.TRCPT1) {
    sendTO++;

    if (sendTO) {
      usbp->sendZlp(COM_EP_OUT);
      currentError = ERROR_COM_TIMEOUT;
    }
    return false;
  }

  if (bytes > COM_SEND_MAX) {
    currentError = ERROR_COM_REQ;
    return false;
  }

  uint32_t sourceAddr = (uint32_t)&source;
  if (sourceAddr == 0) return false;

  endp[COM_EP_OUT]->DeviceDescBank->ADDR.bit.ADDR = sourceAddr;        // Set adress of data to send
  endp[COM_EP_OUT]->DeviceDescBank->PCKSIZE.bit.MULTI_PACKET_SIZE = 0; // Reset packet size counter
  endp[COM_EP_OUT]->DeviceDescBank->PCKSIZE.bit.BYTE_COUNT = bytes;    // Set number of bytes to be sent

  USB->DEVICE.DeviceEndpoint->EPINTFLAG.bit.TRCPT1 = 1;                // Clear transfer complete flag
  USB->DEVICE.DeviceEndpoint->EPSTATUSSET.bit.BK1RDY = 1;              // Set "ready" status

  sendTO.start(true);
  return true;                  
}

int16_t COM_::bytesSent() {
  return endp[COM_EP_OUT]->DeviceDescBank->PCKSIZE.bit.MULTI_PACKET_SIZE;
}

int16_t COM_::bytesRemaining() {
  return endp[COM_EP_OUT]->DeviceDescBank->PCKSIZE.bit.BYTE_COUNT - bytesSent();
}

bool COM_::abortSend(bool blocking) {
  if (USB->DEVICE.DeviceEndpoint[COM_EP_OUT].EPSTATUS.bit.BK1RDY) return false;

  endp[COM_EP_OUT]->DeviceDescBank->PCKSIZE.bit.MULTI_PACKET_SIZE
    = endp[COM_EP_OUT]->DeviceDescBank->PCKSIZE.bit.BYTE_COUNT;

  if (blocking) {
    Timeout TO(currentSendTO, true);
    while(!USB->DEVICE.DeviceEndpoint[1].EPINTFLAG.bit.TRCPT1) {
      if(TO++ <= 0) break;
    }
  }
  return true;
}


bool COM_::recieve(void *destination, int16_t bytes) {

  if (bytes < usbp->available(COM_EP_OUT)) return false;

  if (bytes > COM_RECIEVE_MAX) {
    currentError = ERROR_COM_REQ;
    return false;
  }
  
  usbp->recv(COM_EP_IN, destination, bytes);
} 


int16_t COM_::available() { return usbp->available(COM_EP_OUT); }


bool COM_::busy() {
  if (USB->DEVICE.DeviceEndpoint[3].EPSTATUS.bit.BK1RDY
  || !USB->DEVICE.DeviceEndpoint[3].EPINTFLAG.bit.TRCPT1) {
    return true;
  } else {
    return false;
  }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> COM PRIVATE METHODS
///////////////////////////////////////////////////////////////////////////////////////////////////

COM_::COM_() {
  usbp = &USBDevice;
}

void COM_::resetFields() {

}





