
#include <COM.h>

COM_ &COM;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> COM INTERRUPT HANDLER
///////////////////////////////////////////////////////////////////////////////////////////////////

void COMHandler(void) {   
  // If ended -> call default handler
  if (!COM.begun) COM.usbp->ISRHandler();

  uint8_t interruptReason = COM_REASON_UNKNOWN;        
  bool callbackValid = true;
  bool readyRecv = false;  
  bool readySend = false;   

  // Interrupt on out endpoint
  if (USB->DEVICE.EPINTSMRY.reg & (1 << COM_EP_OUT)) {
    readySend = true;

    // Transfer complete flag
    if (USB->DEVICE.DeviceEndpoint[COM_EP_OUT].EPINTFLAG.bit.TRCPT1) {
      interruptReason = COM_REASON_SEND_COMPLETE;

    // Transfer fail flag
    } else if (USB->DEVICE.DeviceEndpoint[COM_EP_OUT].EPINTFLAG.bit.TRFAIL1) {
      USB->DEVICE.DeviceEndpoint[COM_EP_OUT].EPINTFLAG.bit.TRFAIL1 = 1;
      COM.currentError = ERROR_COM_SEND;
      interruptReason = COM_REASON_SEND_FAIL;

      // Is transfer fail due to ram access error?
      if (USB->DEVICE.INTFLAG.bit.RAMACER) {
        USB->DEVICE.INTFLAG.bit.RAMACER = 1; 
        COM.currentError = ERROR_COM_MEM;  
      }
    }
  // Interrupt on in endpoint
  } else if (USB->DEVICE.EPINTSMRY.reg & (1 << COM_EP_IN)) {
    readyRecv = true;

    // Transfer recieved flag -> shift recieve addr & req next packet
    if (USB->DEVICE.DeviceEndpoint[COM_EP_IN].EPINTFLAG.bit.TRCPT0) {
      interruptReason = COM_REASON_RECEIVE_READY;

    // Transfer recieve fail flag
    } else if (USB->DEVICE.DeviceEndpoint[COM_EP_IN].EPINTFLAG.bit.TRFAIL0) {
      USB->DEVICE.DeviceEndpoint[COM_EP_IN].EPINTFLAG.bit.TRFAIL0 = 1;
      COM.currentError = ERROR_COM_RECEIVE;
      interruptReason = COM_REASON_RECEIVE_FAIL;
  
      // Transfer fail due to buffer overflow?
      if (COM.endp[COM_EP_IN]->DeviceDescBank->STATUS_BK.bit.ERRORFLOW == 1) { // MAYBE ADD A COM.FLUSH(0) HERE????
        COM.currentError = ERROR_COM_MEM;
      }
    }

    if (COM.rxiActive) {
      uint16_t num = UDIV_CEIL(COM.endp[COM_EP_IN]->DeviceDescBank->
        PCKSIZE.bit.BYTE_COUNT, COM_PACKET_SIZE); 
      COM.RXi = (COM.RXi + num <= COM_RX_PACKETS * COM_PACKET_SIZE) ? num : COM.RXi;
    }
    COM.rxiActive = true;

  // Start of frame flag (every roughly ~1ms)
  } else if (USB->DEVICE.INTFLAG.bit.SOF) {
    USB->DEVICE.INTFLAG.bit.SOF = 1;
    interruptReason = COM_REASON_SOF;

  // Reset req flag
  } else {
    if (USB->DEVICE.DeviceEndpoint[COM_EP_CTRL].EPINTFLAG.bit.RXSTP) {
      interruptReason = COM_REASON_RESET;
      COM.usbp->ISRHandler();
      COM.initEP();
    } else {
      COM.usbp->ISRHandler(); // Else -> Unknown interrupt
    }  
  }
  if ((COM.cbrMask & (1 << interruptReason)) == 1) {
    (*COM.callback)(interruptReason);
  }
  // Ready send/recieve after callback
  if (readyRecv) {
    USB->DEVICE.DeviceEndpoint[COM_EP_IN].EPSTATUSSET.bit.BK0RDY = 1; // Set ready status
    USB->DEVICE.DeviceEndpoint[COM_EP_IN].EPINTENCLR.bit.TRCPT0 = 1;  // Disable recieve complete interrupt
  }
  if (readySend) {
    USB->DEVICE.DeviceEndpoint[COM_EP_OUT].EPSTATUSCLR.bit.BK1RDY = 1; // Clear ready status   
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> COM PUBLIC METHODS
///////////////////////////////////////////////////////////////////////////////////////////////////

bool COM_::begin(USBDeviceClass *usbp) {

  // Check exceptions
  if (begun) return true;
  if (usbp == nullptr) return false;
  if (!usbp->configured()) return false;

  resetFields();
  settings.setDefault();
  this->usbp = usbp;
  begun = true;
  
  // Get endpoint descriptors
  for (int16_t i = 0; i < COM_EP_COUNT; i++) {
    endp[i] = (UsbDeviceDescriptor*)USB->DEVICE.DESCADD.bit.DESCADD 
      + (sizeof(UsbDeviceDescriptor) * i);

    if (endp[i] == nullptr) {
      currentError = ERROR_COM_SYS;
      return false;
    }
  }
  USB_SetHandler(&COMHandler);
  return true;
}

bool COM_::end() {
  if (!begun) return true;
  begun = false;
  
  USB->DEVICE.CTRLA.bit.ENABLE = 0;
  while(USB->DEVICE.SYNCBUSY.bit.ENABLE);
  resetFields();
  usbp->init();
  usbp->attach();
  return true;
}

bool COM_::sendPackets(void *source, uint16_t numPackets) {
  if (!begun) return false;
  
  sendTO.start(false);
  if (numPackets <= 0) return false;
  
  // If sendBusy & timeout -> send ZLP
  if (sendBusy()) {
    if (sendTO++ == 0) {
      endp[COM_EP_OUT]->DeviceDescBank->PCKSIZE.bit.BYTE_COUNT = 0;
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
  if (!begun) return false;
  if (!USB->DEVICE.DeviceEndpoint[COM_EP_OUT].EPINTFLAG.bit.TRCPT1
  || USB->DEVICE.DeviceEndpoint[COM_EP_OUT].EPSTATUS.bit.BK1RDY) {
    return true;
  }
  return false;
}

int16_t COM_::packetsSent() {
  if (!begun) return -1;
  return endp[COM_EP_OUT]->DeviceDescBank->
    PCKSIZE.bit.MULTI_PACKET_SIZE / COM_PACKET_SIZE;
}

int16_t COM_::packetsRemaining() {
  if (!begun) return -1;
  return endp[COM_EP_OUT]->DeviceDescBank->
    PCKSIZE.bit.BYTE_COUNT / COM_PACKET_SIZE - packetsSent();
}

bool COM_::abortSend() {
  if (!sendBusy()) return true;
  int16_t transferSize = endp[COM_EP_OUT]->DeviceDescBank->PCKSIZE.bit.BYTE_COUNT; // Get send total bytes
  endp[COM_EP_OUT]->DeviceDescBank->PCKSIZE.bit.MULTI_PACKET_SIZE = transferSize;  // Set total bytes = sent bytes
  return true;
} 

int16_t COM_::recievePackets(void *destination, uint16_t numPackets, bool forceRecieve) {
  if (!begun) return -1;

  if (destination == nullptr) return -1;
  if (USB->DEVICE.DeviceEndpoint[COM_EP_IN].EPINTFLAG.bit.TRCPT0 == 0 
    && !forceRecieve) return 0;

  if (available(true)) {
    if (numPackets > available(true)) {
      if (enforceNumPackets) return 0;
      numPackets = available(true);

    } else if (numPackets == 0) {
      numPackets = COM_DEFAULT_RECIEVE;
    }
    // Transfer requested bytes from RX
    uint16_t reqBytes = numPackets * COM_PACKET_SIZE;
    memcpy(&destination, RX + (RXi - reqBytes), reqBytes);
    RXi -= reqBytes;
  }

  if (RXi <= 0) {
    resetSize(COM_EP_IN);
    RXi = 0;
  }
  return true;
}

bool COM_::request(void *customDest, uint16_t numPackets, bool forceReq) {
  if (!begun) return false;

  // Handle exceptions
  if (RXi > COM_RX_PACKETS - 1) {
    currentError = ERROR_COM_REQ;
    return false;
  }
  if (requestPending()) {
    if (forceReq) cancelRequest();
    else return false;
  }

  // Determine packet count for recieve
  uint16_t bytes = (!numPackets ? COM_DEFAULT_REQ : numPackets) * COM_PACKET_SIZE;

  if (customDest == nullptr && numPackets != 0 
  && RXi + numPackets > COM_RX_PACKETS) {
    if (enforceNumPackets) return false;
    bytes = (COM_RX_PACKETS - RXi) * COM_PACKET_SIZE; 
  }

  resetSize(COM_EP_IN); // Must reset sizes to initiate enable recieve

  endp[COM_EP_IN]->DeviceDescBank->ADDR.bit.ADDR 
    = (customDest == nullptr) ? (uint32_t)RX + bytes : (uint32_t)customDest; // Set destination
  endp[COM_EP_IN]->DeviceDescBank->PCKSIZE.bit.MULTI_PACKET_SIZE = bytes;    // Set packet count
      
  // Enable recieve
  USB->DEVICE.DeviceEndpoint[COM_EP_IN].EPSTATUSCLR.bit.BK0RDY = 1; // Clear ready status
  USB->DEVICE.DeviceEndpoint[COM_EP_IN].EPINTFLAG.bit.TRCPT0 = 1;   // Clear recieved flag
  USB->DEVICE.DeviceEndpoint[COM_EP_IN].EPINTENSET.bit.TRCPT0 = 1;  // Enable transfer complete interrupt

  rxiLast = RXi;
  return true;
}

bool COM_::flush(int16_t numPackets) {
  if (!begun) return false;

  CLAMP(numPackets, -available(true), available(true));
  uint16_t reqBytes = abs(numPackets) * COM_PACKET_SIZE;
  uint16_t absBytes = available(false);

  if (abs(numPackets) == available(true)) { // Clear entire RX buffer
      memset(RX, 0, sizeof(RX));
      RXi = 0;

  } else if (numPackets == 0) { // Clear most packets in RX buffer that wer last recieved
    if (rxiLast != RXi) { 
      memset(RX + rxiLast, 0, (COM_RX_PACKETS - rxiLast) * COM_PACKET_SIZE);
    } else {
      memset(RX + RXi, 0, (COM_RX_PACKETS - RXi) * COM_PACKET_SIZE);
    }
    RXi = rxiLast;

  } else { // Else -> removing packets from top or bottom of RX buffer
    if (numPackets < 0) {
      memmove(RX + reqBytes, RX, absBytes - reqBytes);
    }
    memset(RX + (absBytes - reqBytes), 0, reqBytes);
    RXi -= abs(numPackets);
  } 
  return true;
}

int16_t COM_::packetsRecieved() { 
  if (!begun) return -1;
  uint16_t n = UDIV_CEIL(endp[COM_EP_IN]->DeviceDescBank->PCKSIZE.bit.BYTE_COUNT,
    COM_PACKET_SIZE);
  return (requestPending() ? n : 0);
}

void COM_::cancelRequest() {
  if (!begun) return;
  // Dissable interrupts
  USB->DEVICE.DeviceEndpoint[COM_EP_IN].EPINTENCLR.bit.TRCPT0 = 1;
  USB->DEVICE.DeviceEndpoint[COM_EP_IN].EPINTENCLR.bit.TRFAIL0 = 1;

  // Record current count & reset it.
  uint16_t count = endp[COM_EP_IN]->DeviceDescBank->PCKSIZE.bit.BYTE_COUNT;
  endp[COM_EP_IN]->DeviceDescBank->PCKSIZE.bit.MULTI_PACKET_SIZE = count;

  // Set ready status -> stops transfer
  USB->DEVICE.DeviceEndpoint[COM_EP_IN].EPSTATUSSET.bit.BK0RDY = 1; 

  // Set recieve buffer (RX) index
  RXi += count;
  if (RXi >= COM_RX_PACKETS) {
    RXi = COM_RX_PACKETS - 1;
    currentError = ERROR_COM_RECEIVE;
  }
  // Re-enable interrupts
  USB->DEVICE.DeviceEndpoint[COM_EP_IN].EPINTENCLR.bit.TRCPT0 = 1;
  USB->DEVICE.DeviceEndpoint[COM_EP_IN].EPINTENCLR.bit.TRFAIL0 = 1;
  rxiActive = true;
}

bool COM_::requestPending() {
  if (!begun) return false;
  return (USB->DEVICE.DeviceEndpoint[COM_EP_IN].EPINTFLAG.bit.TRCPT0 == 0
  || USB->DEVICE.DeviceEndpoint[COM_EP_IN].EPSTATUS.bit.BK0RDY == 0);
}

int32_t COM_::getFrameCount(bool getMicroFrameCount) { 
  if (!begun) return 0;
  return (getMicroFrameCount ? USB->DEVICE.FNUM.bit.MFNUM : USB->DEVICE.FNUM.bit.FNUM); 
}

int16_t COM_::available(bool packets) {
  if (!begun) return -1;
  uint16_t bytes = RXi * COM_PACKET_SIZE;
  if (requestPending()) {
    bytes += endp[COM_EP_IN]->DeviceDescBank->PCKSIZE.bit.BYTE_COUNT;
  }
  return (packets ? UDIV_CEIL(bytes, COM_PACKET_SIZE) : bytes);
}

int16_t COM_::getPeripheralState() { 
  if (!begun) return -1;
  return USB->DEVICE.FSMSTATUS.bit.FSMSTATE; 
}

uint8_t *COM_::inspectPacket(uint16_t packetIndex) {
  if (!begun) return nullptr; 
  CLAMP(packetIndex, 0, RXi - 1);
  return &RX[(RXi - 1) - packetIndex]; 
}

ERROR_ID COM_::getError() { 
  if (!begun) return ERROR_UNKNOWN;
  return currentError; 
}

void COM_::clearError() { 
  if (!begun) return;
  currentError = ERROR_NONE; 
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> COM PRIVATE METHODS
///////////////////////////////////////////////////////////////////////////////////////////////////

COM_::COM_() : sendTO(COM_DEFAULT_TIMEOUT, false), otherTO(COM_DEFAULT_TIMEOUT, false) {
  usbp = &USBDevice;
  resetFields();
  settings.setDefault();
}

void COM_::resetFields() {
  memset(RX, 0, sizeof(RX));
  sendTO.setTimeout(COM_DEFAULT_TIMEOUT);
  otherTO.setTimeout(COM_DEFAULT_TIMEOUT);
  RXi = 0;
  rxiLast = 0;
  rxiActive = true;
  currentError = ERROR_NONE;
  begun = false;
}

void COM_::resetSize(int16_t endpoint) {
  endp[endpoint]->DeviceDescBank->PCKSIZE.bit.BYTE_COUNT = 0;
  endp[endpoint]->DeviceDescBank->PCKSIZE.bit.MULTI_PACKET_SIZE = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> COM SETTINGS
///////////////////////////////////////////////////////////////////////////////////////////////////

COM_::COMSettings &COM_::COMSettings::setServiceQuality(int16_t serviceQualityLevel) {
  CLAMP(serviceQualityLevel, 0, COM_MAX_SQ);
  USB->DEVICE.QOSCTRL.bit.DQOS = serviceQualityLevel;
  return *this;
}

COM_::COMSettings &COM_::COMSettings::setCallbackConfig(bool enableRecvRdy, bool enableRecvFail, 
  bool enableSendCompl, bool enableSendFail, bool enableRst, bool enableSOF) {
  uint8_t msk = super->cbrMask;
  msk = 0;

  if (enableRecvRdy) 
    msk |= (1 << COM_REASON_RECEIVE_READY);
  if (enableRecvFail) 
    msk |= (1 << COM_REASON_RECEIVE_FAIL);
  if (enableSendCompl) 
    msk |= (1 << COM_REASON_SEND_COMPLETE);
  if (enableSendFail) 
    msk |= (1 << COM_REASON_SEND_FAIL);
  if (enableRst) 
    msk |= (1 << COM_REASON_RESET);
  if (enableSOF) 
    msk |= (1 << COM_REASON_SOF);
  return *this;
}

COM_::COMSettings &COM_::COMSettings::setCallback(COMCallback *callback) {
  super->callback = callback;
  return *this;
}

COM_::COMSettings &COM_::COMSettings::setTimeout(uint32_t timeout) {
  super->STOtime = timeout;
  super->OTOtime = timeout;
  return *this;
}

COM_::COMSettings &COM_::COMSettings::setEnforceNumPacketConfig(bool enforceNumPackets) {
  super->enforceNumPackets = enforceNumPackets;
  return *this;
}

void COM_::COMSettings::setDefault() {
  USB->DEVICE.QOSCTRL.bit.DQOS = COM_DEFAULT_SQ;
  super->cbrMask = 0;
  super->cbrMask |= COM_DEFAULT_CBRMASK;
  super->callback = COM_DEFAULT_CALLBACK;
  super->STOtime = COM_DEFAULT_TIMEOUT;
  super->OTOtime = COM_DEFAULT_TIMEOUT;
  super->enforceNumPackets = COM_DEFAULT_ENFORCE_NUM;
}





