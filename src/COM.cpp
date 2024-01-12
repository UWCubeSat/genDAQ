
#include <COM.h>

COM_ &COM;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> COM INTERRUPT HANDLER
///////////////////////////////////////////////////////////////////////////////////////////////////

void COMHandler(void) {   
  uint8_t interruptReason = COM_REASON_UNKNOWN;        
  bool callbackValid = true;     

  if (USB->DEVICE.INTFLAG.bit.RAMACER) {
    USB->DEVICE.INTFLAG.bit.RAMACER = 1;
    COM.currentError = ERROR_COM_SYS;
    interruptReason = COM_REASON_RAM_ERROR;      

  } else if (USB->DEVICE.EPINTSMRY.reg & (1 << COM_EP_OUT)) {

    if (USB->DEVICE.DeviceEndpoint[COM_EP_OUT].EPINTFLAG.bit.TRCPT1) {
      USB->DEVICE.DeviceEndpoint[COM_EP_OUT].EPSTATUSSET.bit.BK1RDY = 1; // Ready next transfer (BANK 1)
      USB->DEVICE.DeviceEndpoint[COM_EP_OUT].EPINTFLAG.bit.TRCPT1 = 1;   // Clear transfer complete flag (BANK 1)
      interruptReason = COM_REASON_SEND_COMPLETE;

    } else if (USB->DEVICE.DeviceEndpoint[COM_EP_OUT].EPINTFLAG.bit.TRFAIL1) {
      USB->DEVICE.DeviceEndpoint[COM_EP_OUT].EPINTFLAG.bit.TRFAIL1 = 1;
      COM.currentError = ERROR_COM_SEND;
      interruptReason = COM_REASON_SEND_FAIL;
    }

  } else if (USB->DEVICE.EPINTSMRY.reg & (1 << COM_EP_IN)) {

    if (USB->DEVICE.DeviceEndpoint[COM_EP_IN].EPINTFLAG.bit.TRCPT0) {
      USB->DEVICE.DeviceEndpoint[COM_EP_IN].EPINTFLAG.bit.TRCPT0 = 1;
      interruptReason = COM_REASON_RECEIVE_READY;

    } else if (USB->DEVICE.DeviceEndpoint[COM_EP_IN].EPINTFLAG.bit.TRFAIL0) {
      USB->DEVICE.DeviceEndpoint[COM_EP_IN].EPINTFLAG.bit.TRFAIL0 = 1;
      COM.currentError = ERROR_COM_RECEIVE;
      interruptReason = COM_REASON_RECEIVE_FAIL;
    }

  } else if (USB->DEVICE.INTFLAG.bit.SOF) {
    USB->DEVICE.INTFLAG.bit.SOF = 1;
    interruptReason = COM_REASON_SOF;

  } else {
    if (USB->DEVICE.DeviceEndpoint[0].EPINTFLAG.bit.RXSTP) {
      interruptReason = COM_REASON_RESET;
    }  
    COM.usbp->ISRHandler(); // May need to call handler after this...
  }

  // Call callback (if applicable)
  if (COM.cbrMask & (1 << interruptReason) == 1) {
    (*COM.callback)(interruptReason);
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
  sendTO.setTimeout(COM_DEFAULT_TIMEOUT);
  otherTO.setTimeout(COM_DEFAULT_TIMEOUT);
  USB_SetHandler(&COMHandler);
}

bool COM_::send(void *source, uint8_t bytes, bool cacheData = true) {
  sendTO.start(false);
  
  // Has previous message finished sending?
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
    otherTO.start(true);
    while(!USB->DEVICE.DeviceEndpoint[1].EPINTFLAG.bit.TRCPT1) {
      otherTO++;
      if(otherTO) break;
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
 // TO DO
}







