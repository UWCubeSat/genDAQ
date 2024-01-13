
#include <COM.h>

COM_ &COM;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> COM INTERRUPT HANDLER
///////////////////////////////////////////////////////////////////////////////////////////////////

void COMHandler(void) {   
  uint8_t interruptReason = COM_REASON_UNKNOWN;        
  bool callbackValid = true;     

  // Ram access error
  if (USB->DEVICE.INTFLAG.bit.RAMACER) {
    USB->DEVICE.INTFLAG.bit.RAMACER = 1;
    COM.currentError = ERROR_COM_SYS;
    interruptReason = COM_REASON_RAM_ERROR;      

  // Interrupt on out endpoint
  } else if (USB->DEVICE.EPINTSMRY.reg & (1 << COM_EP_OUT)) {

    // Transfer complete flag
    if (USB->DEVICE.DeviceEndpoint[COM_EP_OUT].EPINTFLAG.bit.TRCPT1) {
      USB->DEVICE.DeviceEndpoint[COM_EP_OUT].EPSTATUSCLR.bit.BK1RDY = 1; // Clear ready status   
      interruptReason = COM_REASON_SEND_COMPLETE;

    // Transfer fail flag
    } else if (USB->DEVICE.DeviceEndpoint[COM_EP_OUT].EPINTFLAG.bit.TRFAIL1) {
      USB->DEVICE.DeviceEndpoint[COM_EP_OUT].EPINTFLAG.bit.TRFAIL1 = 1;
      COM.currentError = ERROR_COM_SEND;
      interruptReason = COM_REASON_SEND_FAIL;
    }

  // Interrupt on in endpoint
  } else if (USB->DEVICE.EPINTSMRY.reg & (1 << COM_EP_IN)) {

    // Transfer recieved flag -> shift recieve addr & req next packet
    if (USB->DEVICE.DeviceEndpoint[COM_EP_IN].EPINTFLAG.bit.TRCPT0) {
      interruptReason = COM_REASON_RECEIVE_READY;

    // Transfer recieve fail flag
    } else if (USB->DEVICE.DeviceEndpoint[COM_EP_IN].EPINTFLAG.bit.TRFAIL0) {
      USB->DEVICE.DeviceEndpoint[COM_EP_IN].EPINTFLAG.bit.TRFAIL0 = 1;
      COM.currentError = ERROR_COM_RECEIVE;
      interruptReason = COM_REASON_RECEIVE_FAIL;

      // Flush bad data
      COM.resetSize(COM_EP_IN);
      COM.flush();
    }

    // Clear queued destination
    if (COM.queueDest != 0) {
      COM.queueDest = 0;

    // If NOT custom destination -> update internal buffer index
    } else if (COM.customDest == 0) {
      COM.RXi = COM.endp[COM_EP_IN]->DeviceDescBank->
        PCKSIZE.bit.BYTE_COUNT / COM_PACKET_SIZE; 
    }

  // Start of frame flag (every roughly ~1ms)
  } else if (USB->DEVICE.INTFLAG.bit.SOF) {
    USB->DEVICE.INTFLAG.bit.SOF = 1;
    interruptReason = COM_REASON_SOF;

  // Reset req flag
  } else {
    if (USB->DEVICE.DeviceEndpoint[0].EPINTFLAG.bit.RXSTP) {
      interruptReason = COM_REASON_RESET;
      COM.usbp->ISRHandler();
      COM.initEP();

    // Unknown interrupt
    } else {
      COM.usbp->ISRHandler();
    }  
  }

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

bool COM_::sendPackets(void *source, uint16_t numPackets) {

  sendTO.start(false);
  if (numPackets <= 0) return false;
  
  // If sendBusy & timeout -> send ZLP
  if (sendBusy()) {
    if (sendTO++ == 0) {
      usbp->sendZlp(COM_EP_OUT);
      currentError = ERROR_COM_TIMEOUT;
      abort();
    }
    return false;
  }

  // Handle source ptr & numPackets
  uint32_t sourceAddr = (uint32_t)&source;
  if (numPackets > COM_SEND_MAX_PACKETS || sourceAddr == 0) {
    currentError = ERROR_COM_REQ;
    return false;
  }

  // Re-init endpoint descriptor
  endp[COM_EP_OUT]->DeviceDescBank->ADDR.bit.ADDR = sourceAddr;         // Set adress of data to send
  endp[COM_EP_OUT]->DeviceDescBank->PCKSIZE.bit.BYTE_COUNT = 0;         // Reset byte count
  endp[COM_EP_OUT]->DeviceDescBank->PCKSIZE.bit.MULTI_PACKET_SIZE = 0;  // Reset packet size counter
  
  // Configure endpoint descriptor for send
  endp[COM_EP_OUT]->DeviceDescBank->PCKSIZE.bit.MULTI_PACKET_SIZE       // Set number of packets
    = numPackets;
  endp[COM_EP_OUT]->DeviceDescBank->PCKSIZE.bit.BYTE_COUNT              // Set number of bytes in a packet
    = COM_PACKET_SIZE;
  USB->DEVICE.DeviceEndpoint->EPINTFLAG.bit.TRCPT1 = 1;                 // Clear transfer complete flag
  USB->DEVICE.DeviceEndpoint->EPSTATUSSET.bit.BK1RDY = 1;               // Set "ready" status

  sendTO.start(true);
  return true;                  
}

bool COM_::sendBusy() {
  if (!USB->DEVICE.DeviceEndpoint[COM_EP_OUT].EPINTFLAG.bit.TRCPT1
  || USB->DEVICE.DeviceEndpoint[COM_EP_OUT].EPSTATUS.bit.BK1RDY) {
    return true;
  }
}

int16_t COM_::packetsSent() {
  return endp[COM_EP_OUT]->DeviceDescBank->
    PCKSIZE.bit.MULTI_PACKET_SIZE / COM_PACKET_SIZE;
}

int16_t COM_::packetsRemaining() {
  return endp[COM_EP_OUT]->DeviceDescBank->
    PCKSIZE.bit.BYTE_COUNT / COM_PACKET_SIZE - packetsSent();
}

bool COM_::abort() {
  if (!sendBusy()) return true;
  int16_t transferSize = endp[COM_EP_OUT]->DeviceDescBank->PCKSIZE.bit.BYTE_COUNT; // Get send total bytes
  endp[COM_EP_OUT]->DeviceDescBank->PCKSIZE.bit.MULTI_PACKET_SIZE = transferSize;  // Set total bytes = sent bytes
  return true;
} 

int16_t COM_::recievePackets(void *destination, uint16_t reqPackets) {

  // Handle exceptions
  if (destination == nullptr) return -1;
  if (USB->DEVICE.DeviceEndpoint[COM_EP_IN].EPINTFLAG.bit.TRCPT0 == 0) return 0;

  if (available()) {
    if (reqPackets > available()) {
      if (enforceNumPackets) return false;
      else reqPackets = available();
    }
    // Disable transfer complete interrupt
    USB->DEVICE.DeviceEndpoint[COM_EP_IN].EPINTENCLR.bit.TRCPT0 = 1;

    // Transfer requested bytes from RX
    uint16_t reqBytes = reqPackets * COM_PACKET_SIZE;
    memcpy(&destination, &RX[RXi - reqBytes], reqBytes);
    RXi -= reqBytes;
  }

  if (RXi <= 0) {
    RXi = 0;
    resetSize(COM_EP_IN);

    uint32_t dest = (uint32_t)&RX; 
    if (queueDest != 0) dest = queueDest;
    else if (customDest) dest = customDest;

    // Set destination
    endp[COM_EP_IN]->DeviceDescBank->ADDR.bit.ADDR = dest;

    // Set packet count & set "ready" status
    endp[COM_EP_IN]->DeviceDescBank->PCKSIZE.bit.MULTI_PACKET_SIZE = rxPacketCount;
    USB->DEVICE.DeviceEndpoint[COM_EP_IN].EPSTATUSCLR.bit.BK0RDY = 1;

    // Re-enable transfer complete interrupt & clear flag
    USB->DEVICE.DeviceEndpoint[COM_EP_IN].EPINTFLAG.bit.TRCPT0 = 1;   
    USB->DEVICE.DeviceEndpoint[COM_EP_IN].EPINTENSET.bit.TRCPT0 = 1;
  }
  return true;
}

uint8_t *COM_::inspectPackets(uint16_t packetIndex) {
  CLAMP(packetIndex, 0, available());
  if (packetIndex == 0) return nullptr;
  return &RX[packetIndex];
}

void COM_::flush() {
  endp[COM_EP_IN]->DeviceDescBank->PCKSIZE.bit.BYTE_COUNT = 0;
  memset(&RX, 0, endp[COM_EP_OUT]->DeviceDescBank->PCKSIZE.bit.MULTI_PACKET_SIZE);
  RXi = 0;
}

int16_t COM_::available() { 
  return UDIV_CEIL(RXi , COM_PACKET_SIZE); 
}

int16_t COM_::recieved() { 
  return endp[COM_EP_IN]->DeviceDescBank->PCKSIZE.bit.BYTE_COUNT / COM_PACKET_SIZE;
}

bool COM_::queueDestination(void *destination) {
  if (recievePending()) return false;
  if (destination == nullptr) return false;
  queueDest = (uint32_t)destination;
  return true;
}

bool COM_::recievePending() {
  if (USB->DEVICE.DeviceEndpoint[COM_EP_IN].EPINTFLAG.bit.TRCPT0 == 0) {
    return false;
  } else {
    return true;
  }
}

ERROR_ID COM_::getError() { return currentError; }

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> COM PRIVATE METHODS
///////////////////////////////////////////////////////////////////////////////////////////////////

COM_::COM_() : sendTO(COM_DEFAULT_TIMEOUT, false), otherTO(COM_DEFAULT_TIMEOUT, false) {
  usbp = &USBDevice;
  resetFields();
}

void COM_::resetFields() {
 // TO DO
}

void COM_::resetSize(int16_t endpoint) {
  endp[endpoint]->DeviceDescBank->PCKSIZE.bit.BYTE_COUNT = 0;
  endp[endpoint]->DeviceDescBank->PCKSIZE.bit.MULTI_PACKET_SIZE = 0;
}







