
#include <DMAUtility.h>


static __attribute__((__aligned__(16))) DmacDescriptor 
  primaryDescriptorArray[DMA_MAX_CHANNELS] SECTION_DMAC_DESCRIPTOR,
  writebackDescriptorArray[DMA_MAX_CHANNELS] SECTION_DMAC_DESCRIPTOR;


DMAUtility &DMA;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> DMA UTILITY
///////////////////////////////////////////////////////////////////////////////////////////////////

  



///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> DMA INTERRUPT FUNCTION
///////////////////////////////////////////////////////////////////////////////////////////////////


void DMAC_0_Handler(void) {
  DMAChannel &channel = DMA.getChannel(DMAC->INTPEND.bit.ID);
  DMACallbackFunction callback = channel.callback;
  DMA_CALLBACK_REASON reason = REASON_UNKNOWN;


  // Transfer error
  if(DMAC->INTPEND.bit.TERR) {
    reason = REASON_ERROR;

    // Transfer error -> determine source
    if (DMAC->INTPEND.bit.CRCERR) {
      channel.currentError = DMA_ERROR_CRC;
    } else {
      channel.currentError = DMA_ERROR_TRANSFER;
    }

  // Channel suspended
  } else if (DMAC->INTPEND.bit.SUSP) {

    // Is it due to fetch error or suspend action?
    if (DMAC->INTPEND.bit.FERR) {
      channel.currentError = DMA_ERROR_DESCRIPTOR;
      reason = REASON_ERROR;
    } else {
      reason = REASON_SUSPENDED;
    }

  // Transfer complete
  } else if (DMAC->INTPEND.bit.TCMPL) {
    reason = REASON_TRANSFER_COMPLETE;

    channel.currentDescriptor++;
    if (channel.currentDescriptor == channel.descriptorCount) {
      // Have to consider other modes here...
    }

  } else {

  }

}



///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> DMA DESCRIPTOR METHODS
///////////////////////////////////////////////////////////////////////////////////////////////////

Descriptor::Descriptor () {
  setDefault();
}

Descriptor &Descriptor::setSource(void *source) {
  if (source == nullptr) {
    desc.SRCADDR.bit.SRCADDR = DMA_DEFAULT_SOURCE;
  } else {
    desc.SRCADDR.bit.SRCADDR = (uint32_t)source;
  }
  return *this;
}

Descriptor &Descriptor::setDestination(void *destination) {
  if (destination == nullptr) {
    desc.DSTADDR.bit.DSTADDR = DMA_DEFAULT_DESTINATION;
  } else {
    desc.DSTADDR.bit.DSTADDR = (uint32_t)destination;
  }
  return *this;
}

Descriptor &Descriptor::setTransferAmount(uint16_t byteCount) {
  desc.BTCNT.bit.BTCNT = (uint16_t)byteCount;
  return *this;
}

Descriptor &Descriptor::setDataSize(int16_t byteCount) {
  if (byteCount == 1) {
    desc.BTCTRL.bit.BEATSIZE = DMAC_BTCTRL_BEATSIZE_BYTE_Val;
  } else if (byteCount == 2) {
    desc.BTCTRL.bit.BEATSIZE = DMAC_BTCTRL_BEATSIZE_HWORD_Val;
  } else if (byteCount == 4) {
    desc.BTCTRL.bit.BEATSIZE = DMAC_BTCTRL_BEATSIZE_WORD_Val;
  } else {
    desc.BTCTRL.bit.BEATSIZE = DMA_DEFAULT_DATA_SIZE;
  }
  return *this;
}

Descriptor &Descriptor::setIncrementConfig(bool incrementSource, bool incrementDestination) {
  if (incrementSource) {
    desc.BTCTRL.bit.SRCINC = 1;
  } else {
    desc.BTCTRL.bit.SRCINC = 0;
  }
  if (incrementDestination) {
    desc.BTCTRL.bit.DSTINC = 1;
  } else {
    desc.BTCTRL.bit.DSTINC = 0;
  }
  return *this;
}

Descriptor &Descriptor::setIncrementModifier(DMA_TARGET target, int16_t modifier) {
  if (modifier > DMAC_BTCTRL_STEPSIZE_X128_Val || modifier < DMAC_BTCTRL_STEPSIZE_X1_Val) {
    desc.BTCTRL.bit.STEPSIZE = DMA_DEFAULT_STEPSIZE;
    desc.BTCTRL.bit.STEPSEL = DMA_DEFAULT_STEPSELECTION;
  } else {
    if (target = SOURCE) {
      desc.BTCTRL.bit.STEPSEL = DMAC_BTCTRL_STEPSEL_SRC_Val;
    } else {
      desc.BTCTRL.bit.STEPSEL = DMAC_BTCTRL_STEPSEL_DST_Val;
    }
    desc.BTCTRL.bit.STEPSIZE = modifier;
  }
  return *this;
}

Descriptor &Descriptor::setAction(DMA_ACTION action) {
  desc.BTCTRL.bit.BLOCKACT = (uint8_t)action;
}

void Descriptor::setDefault() {
  desc.BTCNT.bit.BTCNT = DMA_DEFAULT_TRANSFER_AMOUNT;
  desc.DSTADDR.bit.DSTADDR = DMA_DEFAULT_DESTINATION;
  desc.SRCADDR.bit.SRCADDR = DMA_DEFAULT_SOURCE;
  desc.BTCTRL.bit.BEATSIZE = DMA_DEFAULT_DATA_SIZE;
  desc.BTCTRL.bit.BLOCKACT = DMA_DEFAULT_ACTION;
  desc.BTCTRL.bit.DSTINC = DMA_DEFAULT_INCREMENT_DESTINATION;
  desc.BTCTRL.bit.SRCINC = DMA_DEFAULT_INCREMENT_SOURCE;
  desc.BTCTRL.bit.STEPSEL = DMA_DEFAULT_STEPSELECTION;
  desc.BTCTRL.bit.STEPSIZE = DMA_DEFAULT_STEPSIZE;
  desc.BTCTRL.bit.VALID = 1;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> DMA CHANNEL METHODS
///////////////////////////////////////////////////////////////////////////////////////////////////


DMAChannel::DMAChannel(int16_t channelIndex) : channelIndex(channelIndex) {}


bool DMAChannel::setDescriptors(Descriptor **descriptorArray, int16_t count, bool loop) {
  // Check for exceptions
  if (descriptorArray == nullptr) return false;
  for (int16_t i = 0; i < count; i++) {
    if (descriptorArray[i] == nullptr) return false;
  }
  // current descriptor will actually be the previous one.
  if (descriptorCount > 0) clearDescriptors();
  DmacDescriptor *currentDescriptor = nullptr;

  // Copy first given descriptor into primariy descriptor array
  memcpy(&primaryDescriptorArray[channelIndex], &descriptorArray[0]->desc, sizeof(DmacDescriptor));
  currentDescriptor = &primaryDescriptorArray[channelIndex];

  if (count > 1) {

    // Iterate through given descriptors & create/link them
    for (int16_t i = 0; i < count; i++) {
      DmacDescriptor *newDescriptor = new DmacDescriptor;
      memcpy(newDescriptor, &descriptorArray[i]->desc, sizeof(DmacDescriptor));

      currentDescriptor->DESCADDR.bit.DESCADDR = (uint32_t)newDescriptor;
      currentDescriptor = newDescriptor;
    }
  }

  // Link the "current descriptor" to the first one if loop is true
  if (loop) {
    currentDescriptor->DESCADDR.bit.DESCADDR = (uint32_t)&primaryDescriptorArray[channelIndex];
  }
  descriptorCount = count;
  return true;
}


bool DMAChannel::setDescriptor(Descriptor *descriptor, bool loop) {
  Descriptor *descriptorArr[] = { descriptor };
  return setDescriptors(descriptorArr, 1, loop);
}


bool DMAChannel::trigger() {
  // Check for exceptions
  if (isPending()
      || getStatus() == DMA_CHANNEL_DISABLED
      || getStatus() == DMA_CHANNEL_ERROR) {
    return false;
  }
  // Trigger the channel
  DMAC->SWTRIGCTRL.reg |= (1 << channelIndex); 

  // Set either pend or trigger flag based on result of trigger
  if (getStatus() == DMA_CHANNEL_BUSY) {
    swTriggerFlag = true;
  } else {
    swPendFlag = true;
  }
  return true;
}


bool DMAChannel::suspend(bool blocking) {
  if (getStatus() == DMA_CHANNEL_DISABLED
      || getStatus() == DMA_CHANNEL_ERROR) {
    return false;
  }
  // Send a suspend cmd 
  DMAC->Channel[channelIndex].CHCTRLB.bit.CMD = DMAC_CHCTRLB_CMD_SUSPEND_Val;

  // If blocking wait for interrupt
  if (blocking) {
    while(!DMAC->Channel[channelIndex].CHINTFLAG.bit.SUSP);
  }
  return true;
}


bool DMAChannel::resume() {
  if (getStatus() == DMA_CHANNEL_DISABLED
      || getStatus() == DMA_CHANNEL_ERROR) {
    return false;
  }
  // Send a resume cmd & clear flag
  DMAC->Channel[channelIndex].CHCTRLB.bit.CMD = DMAC_CHCTRLB_CMD_RESUME_Val;
  suspendFlag = false;
  return true;
}


bool DMAChannel::noaction() {
  if (getStatus() == DMA_CHANNEL_DISABLED
      || getStatus() == DMA_CHANNEL_ERROR) {
    return false;
  }
  // Send a noaction cmd & clear flags
  DMAC->Channel[channelIndex].CHCTRLB.bit.CMD = DMAC_CHCTRLB_CMD_NOACT_Val;
  suspendFlag = false;
  return true;
}


void DMAChannel::disable(bool blocking) {
  DMAC->Channel[channelIndex].CHCTRLA.bit.ENABLE = 0;
  if (blocking) {
    while(DMAC->Channel[channelIndex].CHSTATUS.bit.BUSY);
  }
}


bool DMAChannel::enable() {
  if (descriptorCount == 0) {
    return false;
  }
  DMAC->Channel[channelIndex].CHCTRLA.bit.ENABLE = 1;
  return true;
}


bool DMAChannel::enableTrigger() {
  if (externalTrigger != TRIGGER_SOFTWARE) {
    DMAC->Channel[channelIndex].CHCTRLA.bit.TRIGSRC = (uint8_t)externalTrigger;
    return true;
  } else {
    return false;
  }
}

bool DMAChannel::disableTrigger() {
  if (externalTrigger != TRIGGER_SOFTWARE) {
    DMAC->Channel[channelIndex].CHCTRLA.bit.TRIGSRC = (uint8_t)TRIGGER_SOFTWARE;
  } else {
    return false;
  }
}


void DMAChannel::clear() {
  externalTrigger = TRIGGER_SOFTWARE;
  descriptorCount = 0;
  swPendFlag = false;
  swTriggerFlag = false;
  transferErrorFlag = false;
  suspendFlag = false;
  currentDescriptor = 0;

  clearDescriptors();

  // Disable the channel and call a soft reset of all registers
  DMAC->Channel[channelIndex].CHCTRLA.bit.ENABLE = 0;
  while(DMAC->Channel[channelIndex].CHCTRLA.bit.ENABLE);
  DMAC->Channel[channelIndex].CHCTRLA.bit.SWRST = 1;
  while(DMAC->Channel[channelIndex].CHCTRLA.bit.SWRST);
}


bool DMAChannel::isPending() { return getPending() > 0; }


bool DMAChannel::isBusy() { 
return (DMAC->Channel[channelIndex].CHSTATUS.bit.BUSY 
    || DMAC->Channel[channelIndex].CHSTATUS.bit.PEND);
}


int16_t DMAChannel::getCurrentDescriptor() { return currentDescriptor; }


bool DMAChannel::getTriggerEnabled() {
  return DMAC->Channel[channelIndex].CHCTRLA.bit.TRIGSRC != (uint8_t)TRIGGER_SOFTWARE;
}


int16_t DMAChannel::getPending() {
  if (DMAC->PENDCH.reg & ~(1 << channelIndex) == 1) {
    if (swPendFlag) {
      return 1;
    } else if (externalTrigger != TRIGGER_SOFTWARE){
      return 2;
    }
  }
  return 0;
}


DMA_STATUS DMAChannel::getStatus() {
  if (DMAC->Channel[channelIndex].CHSTATUS.bit.BUSY
  || DMAC->Channel[channelIndex].CHSTATUS.bit.PEND) {
    return DMA_CHANNEL_BUSY;

  } else if (DMAC->Channel[channelIndex].CHSTATUS.bit.FERR
  || DMAC->Channel[channelIndex].CHSTATUS.bit.CRCERR
  || transferErrorFlag) {
    return DMA_CHANNEL_ERROR;

  } else if (suspendFlag) {
    return DMA_CHANNEL_SUSPENDED;

  } else if (DMAC->Channel[channelIndex].CHCTRLA.bit.ENABLE) {
    return DMA_CHANNEL_IDLE;

  } else {
    return DMA_CHANNEL_DISABLED;
  }
}


void DMAChannel::clearDescriptors() {
  if (descriptorCount > 1) {
    DmacDescriptor *currentDescriptor 
      = (DmacDescriptor*)primaryDescriptorArray[channelIndex].DESCADDR.bit.DESCADDR;
    memset(&primaryDescriptorArray[channelIndex], 0, sizeof(DmacDescriptor));

      for (int16_t i = 0; i < descriptorCount - 1; i++) {
        DmacDescriptor *nextDescriptor
          = (DmacDescriptor*)primaryDescriptorArray[channelIndex].DESCADDR.bit.DESCADDR;
        delete currentDescriptor;
        currentDescriptor = nextDescriptor;
      }
      delete currentDescriptor;
      currentDescriptor = nullptr;
  
  } else if (descriptorCount == 1) {
    memset(&primaryDescriptorArray[channelIndex], 0, sizeof(DmacDescriptor));
  }
}
