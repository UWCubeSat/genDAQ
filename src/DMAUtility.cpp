
#include <DMAUtility.h>

//// FORWARD DECLARATIONS ////
int16_t getTrigger(bool swTriggerFlag);
void DMAC_0_Handler(void);

void DMAC_1_Handler(void) __attribute__((weak, alias("DMAC_0_Handler")));  // Re-route all handlers to handler 0
void DMAC_2_Handler(void) __attribute__((weak, alias("DMAC_0_Handler")));
void DMAC_3_Handler(void) __attribute__((weak, alias("DMAC_0_Handler")));
void DMAC_4_Handler(void) __attribute__((weak, alias("DMAC_0_Handler")));


static __attribute__((__aligned__(16))) DmacDescriptor 
  primaryDescriptorArray[DMA_MAX_CHANNELS] SECTION_DMAC_DESCRIPTOR,
  writebackDescriptorArray[DMA_MAX_CHANNELS] SECTION_DMAC_DESCRIPTOR;

DMAUtility &DMA;

bool DMAUtility::begun = false;
int16_t DMAUtility::currentChannel = 0;
DMAChannel DMAUtility::channelArray[DMA_MAX_CHANNELS] = { 
  DMAChannel(0), DMAChannel(1), DMAChannel(2), DMAChannel(3), DMAChannel(4),
  DMAChannel(5), DMAChannel(6), DMAChannel(7), DMAChannel(8), DMAChannel(9),
  DMAChannel(10), DMAChannel(11), DMAChannel(12), DMAChannel(13), DMAChannel(14),
  DMAChannel(15)
};

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> DMA UTILITY
///////////////////////////////////////////////////////////////////////////////////////////////////

void DMAUtility::begin() {
  end(); // Reset DMA  

  // Configure DMA settings
  MCLK->AHBMASK.bit.DMAC_ = 1;                                     // Enable DMA clock.
  DMAC->BASEADDR.bit.BASEADDR = (uint32_t)primaryDescriptorArray;  // Set primary descriptor storage SRAM address
  DMAC->WRBADDR.bit.WRBADDR = (uint32_t)writebackDescriptorArray;  // Set writeback descriptor storage SRAM address              

  // Enable all priority levels
  for (int16_t i = 0; i < DMA_PRIORITY_LVL_COUNT; i++) {
    DMAC->CTRL.reg |= (1 << (DMAC_CTRL_LVLEN_Pos + i)); 
  }

  // Enable round-robin schedualing for all priority lvls
  for (int16_t i = 1; i <= DMAC_LVL_NUM; i++) {
    DMAC->PRICTRL0.reg |= (1 << (((DMAC_PRICTRL0_RRLVLEN0_Pos + 1) * i) - 1));
  }

  // Enable interrupts and set to lowest possible priority
  for (int16_t i = 0; i < DMA_IRQ_COUNT; i++) {
    NVIC_SetPriority((IRQn_Type)(DMAC_0_IRQn + i), (1 << __NVIC_PRIO_BITS) - 1);  
    NVIC_EnableIRQ((IRQn_Type)(DMAC_0_IRQn + i));
  }
  // Enable DMA
  DMAC->CTRL.bit.DMAENABLE = 1;
  begun = true; 
}


void DMAUtility::end() {
  // Clear DMA registers
  DMAC->CTRL.bit.DMAENABLE = 0;                   // Disable system -> grants write access
  DMAC->CTRL.bit.SWRST = 1;                       // Reset entire system          
  MCLK->AHBMASK.bit.DMAC_ = 0;                    // Disable clock

  // Disable all interrupts
  for (int16_t i = 0; i < DMA_IRQ_COUNT; i++) {
    NVIC_ClearPendingIRQ((IRQn_Type)(DMAC_0_IRQn + i));  
    NVIC_DisableIRQ((IRQn_Type)(DMAC_0_IRQn + i));
  }
  // Clear all channels
  for (int16_t i = 0; i < DMA_MAX_CHANNELS; i++) {      
    channelArray[i].clear();
  }
  // Reset variables
  currentChannel = 0;
  begun = false;
}


DMAChannel &DMAUtility::getChannel(int16_t channelIndex) {
  CLAMP(channelIndex, 0, DMA_MAX_CHANNELS - 1);
  if (!channelArray[channelIndex].allocated) {
    channelArray[channelIndex].init(-1);
    channelArray[channelIndex].allocated = true;
  }
  return channelArray[channelIndex];
}

DMAChannel &DMAUtility::operator[] (int16_t channelIndex) {
  return getChannel(channelIndex);
}



DMAChannel *DMAUtility::allocateChannel(int16_t ownerID) {
  if (ownerID < -1) ownerID = -1;

  // Iterate through channels to find one that is not allocated
  for (int16_t i = 0; i < DMA_MAX_CHANNELS; i++) {
    if (channelArray[i].allocated == false) {

      // Channel found -> initialize channel
      DMAChannel &allocCH = channelArray[i];
      allocCH.allocated = true;
      allocCH.init(ownerID);
      return &allocCH;
    }
  }
  return nullptr;
}
DMAChannel *DMAUtility::allocateChannel() {
  return allocateChannel(-1);
}


void DMAUtility::freeChannel(int16_t channelIndex) {
  CLAMP(channelIndex, 0, DMA_MAX_CHANNELS - 1);
  DMAChannel &targChannel = channelArray[channelIndex];

  // Clear channel and reset alloc flag/id
  targChannel.clear();
  targChannel.allocated = false;
  targChannel.ownerID = -1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> DMA INTERRUPT FUNCTION
///////////////////////////////////////////////////////////////////////////////////////////////////

int16_t getTrigger(bool swTriggerFlag) {
  if (swTriggerFlag) {
    return 1;
  } else {
    return 2;
  }
  return 0;
}

//////////////////////// HANDLER NEEDS TO BE UPDATED TO CONSIDER BURST BASED TRANSFERS  -> AND OTHER STUFF IS PROB WRONG...

void DMAC_0_Handler(void) {
  DMAChannel &channel = DMA.getChannel(DMAC->INTPEND.bit.ID);
  DMACallbackFunction callback = channel.callback;
  DMA_CALLBACK_REASON reason = REASON_UNKNOWN;
  uint8_t interruptTrigger = 0;

  // Transfer error
  if(DMAC->INTPEND.bit.TERR) {
    reason = REASON_ERROR;

    // Transfer error -> determine source
    if (DMAC->INTPEND.bit.CRCERR) {
      channel.currentError = DMA_ERROR_CRC;
    } else {
      channel.currentError = DMA_ERROR_TRANSFER;
      channel.transferErrorFlag = true;
    }

    // Update most recent active trigger & clear flags (bc disabled)
    interruptTrigger = getTrigger(channel.swTriggerFlag);
    channel.swTriggerFlag = false;
    channel.swPendFlag = false;

  // Channel suspended
  } else if (DMAC->INTPEND.bit.SUSP) {
      channel.suspendFlag = true;

    // Suspend req from client -> Do nothing
    if (DMAC->Channel[channel.channelIndex].CHCTRLB.bit.CMD == DMAC_CHCTRLB_CMD_SUSPEND_Val) {
      goto end;

    // TransferDescriptor error
    } else if (DMAC->INTPEND.bit.FERR) {
      channel.currentError = DMA_ERROR_DESCRIPTOR;
      reason = REASON_ERROR;
      interruptTrigger = getTrigger(channel.swTriggerFlag);

    // Block suspend action -> transfer complete
    } else if (writebackDescriptorArray[channel.channelIndex].BTCTRL.bit.BLOCKACT
        == DMAC_BTCTRL_BLOCKACT_SUSPEND_Val) {
      reason = REASON_SUSPENDED; 
      channel.currentError = DMA_ERROR_NONE;
      interruptTrigger = getTrigger(channel.swTriggerFlag);
      goto transferComplete;
    }

  // Transfer complete
  } else if (DMAC->INTPEND.bit.TCMPL) {
    reason = REASON_TRANSFER_COMPLETE;
    channel.currentError = DMA_ERROR_NONE;
    interruptTrigger = getTrigger(channel.swTriggerFlag);

    transferComplete: {
      // Update trigger & pending flags
      channel.swTriggerFlag = channel.swPendFlag;
      channel.swPendFlag = false;

      // Update descriptor counter
      channel.currentDescriptor++;
      if (channel.currentDescriptor >= channel.descriptorCount) {
        channel.currentDescriptor = 0;
      }
    }
  }
  if (channel.callback != nullptr) {
    callback(reason, channel, channel.currentDescriptor, interruptTrigger, channel.currentError);
  }
  end: 
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> DMA DESCRIPTOR METHODS
///////////////////////////////////////////////////////////////////////////////////////////////////

TransferDescriptor::TransferDescriptor () {
  setDefault();
}

TransferDescriptor &TransferDescriptor::setSource(void *sourcePointer) {
  if (sourcePointer == nullptr) {
    // => ASSERT GOES HERE
  } else {
    uint32_t startAddress = (uint32_t)sourcePointer;
    uint32_t targAddress = 0;
    if (desc.BTCTRL.bit.STEPSEL && desc.BTCTRL.bit.SRCINC) {
      targAddress = startAddress + (desc.BTCNT.bit.BTCNT 
        * (desc.BTCTRL.bit.BEATSIZE + 1) 
        * (1 << desc.BTCTRL.bit.STEPSIZE)); 
    } else {
      targAddress = startAddress + (desc.BTCNT.bit.BTCNT
        * (desc.BTCTRL.bit.BEATSIZE + 1));
    }   
    desc.SRCADDR.bit.SRCADDR = targAddress;
  }
  return *this;
}

TransferDescriptor &TransferDescriptor::setDestination(void *destinationPointer) {
  if (destinationPointer == nullptr) {
    // => ASSERT GOES HERE
  } else {
    uint32_t startAddress = (uint32_t)destinationPointer;
    uint32_t targAddress = 0;
    if (!desc.BTCTRL.bit.STEPSEL && desc.BTCTRL.bit.DSTINC) {
      targAddress = startAddress + (desc.BTCNT.bit.BTCNT
        * (desc.BTCTRL.bit.BEATSIZE + 1)
        * (1 << desc.BTCTRL.bit.STEPSIZE));
    } else {
      targAddress = startAddress + (desc.BTCNT.bit.BTCNT
        * (desc.BTCTRL.bit.BEATSIZE + 1));
    }
    desc.DSTADDR.bit.DSTADDR = targAddress;  
  }
  return *this;
}

TransferDescriptor &TransferDescriptor::setTransferAmount(uint16_t byteCount) {
  desc.BTCNT.bit.BTCNT = (uint16_t)byteCount;
  return *this;
}

TransferDescriptor &TransferDescriptor::setDataSize(int16_t byteCount) {
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

TransferDescriptor &TransferDescriptor::setIncrementConfig(bool incrementSource, bool incrementDestination) {
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

TransferDescriptor &TransferDescriptor::setIncrementModifier(DMA_TARGET target, int16_t modifier) {
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

TransferDescriptor &TransferDescriptor::setAction(DMA_TRANSFER_ACTION action) {
  desc.BTCTRL.bit.BLOCKACT = (uint8_t)action;
}

void TransferDescriptor::setDefault() {
  desc.BTCNT.bit.BTCNT = DMA_DEFAULT_TRANSFER_AMOUNT;
  desc.DSTADDR.bit.DSTADDR = DMA_DEFAULT_DESTINATION;
  desc.SRCADDR.bit.SRCADDR = DMA_DEFAULT_SOURCE;
  desc.BTCTRL.bit.BEATSIZE = DMA_DEFAULT_DATA_SIZE;
  desc.BTCTRL.bit.BLOCKACT = DMA_DEFAULT_TRANSFER_ACTION;
  desc.BTCTRL.bit.DSTINC = DMA_DEFAULT_INCREMENT_DESTINATION;
  desc.BTCTRL.bit.SRCINC = DMA_DEFAULT_INCREMENT_SOURCE;
  desc.BTCTRL.bit.STEPSEL = DMA_DEFAULT_STEPSELECTION;
  desc.BTCTRL.bit.STEPSIZE = DMA_DEFAULT_STEPSIZE;
  desc.BTCTRL.bit.VALID = 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> DMA CHANNEL SETTINGS
///////////////////////////////////////////////////////////////////////////////////////////////////

DMAChannel::ChannelSettings &DMAChannel::ChannelSettings::setTransferThreshold(int16_t elements) {
  elements -= 1; 
  CLAMP(elements, 0, DMAC_CHCTRLA_THRESHOLD_8BEATS_Val);
  DMAC->Channel[super->channelIndex].CHCTRLA.bit.THRESHOLD = elements;
  return *this;
}

DMAChannel::ChannelSettings &DMAChannel::ChannelSettings::setBurstLength(int16_t elements) {
  elements -= 1;
  CLAMP(elements, 0, DMAC_CHCTRLA_BURSTLEN_16BEAT_Val);
  DMAC->Channel[super->channelIndex].CHCTRLA.bit.BURSTLEN = elements;
  return *this;
}

DMAChannel::ChannelSettings &DMAChannel::ChannelSettings::setTriggerAction(DMA_TRIGGER_ACTION action) {
  DMAC->Channel[super->channelIndex].CHCTRLA.bit.TRIGACT = (uint8_t)action;
  return *this;
}

DMAChannel::ChannelSettings &DMAChannel::ChannelSettings::setStandbyConfig(bool enabledDurringStandby) {
  if (enabledDurringStandby) {
    DMAC->Channel[super->channelIndex].CHCTRLA.bit.RUNSTDBY = 1;
  } else {
    DMAC->Channel[super->channelIndex].CHCTRLA.bit.RUNSTDBY = 0;
  }
  return *this;
}

DMAChannel::ChannelSettings &DMAChannel::ChannelSettings::setPriorityLevel(int16_t level) {
  CLAMP(level, 0, 3);
  DMAC->Channel[super->channelIndex].CHPRILVL.bit.PRILVL = level;
  return *this;
}

DMAChannel::ChannelSettings &DMAChannel::ChannelSettings::setCallbackFunction(DMACallbackFunction callback) {
  super->callback = callback;
  return *this;
}

void DMAChannel::ChannelSettings::removeCallbackFunction() {
  super->callback = nullptr;
}

DMAChannel::ChannelSettings &DMAChannel::ChannelSettings::setExternalTrigger(DMA_TRIGGER trigger) {
  DMAC->Channel[super->channelIndex].CHCTRLA.bit.TRIGSRC = (uint8_t)trigger;
  super->externalTrigger = trigger;
  return *this;
}

void DMAChannel::ChannelSettings::setDefault() {
  DMAC->Channel[super->channelIndex].CHCTRLA.bit.BURSTLEN = DMA_DEFAULT_BURST_LENGTH;
  DMAC->Channel[super->channelIndex].CHCTRLA.bit.THRESHOLD = DMA_DEFAULT_TRANSMISSION_THRESHOLD;
  DMAC->Channel[super->channelIndex].CHCTRLA.bit.TRIGACT = DMA_DEFAULT_TRIGGER_ACTION;
  DMAC->Channel[super->channelIndex].CHCTRLA.bit.TRIGSRC = DMA_DEFAULT_TRIGGER_SOURCE;
  DMAC->Channel[super->channelIndex].CHPRILVL.bit.PRILVL = DMA_DEFAULT_PRIORITY_LVL;
  DMAC->Channel[super->channelIndex].CHCTRLA.bit.RUNSTDBY = DMA_DEFAULT_RUN_STANDBY;
  super->callback = nullptr;
  super->externalTrigger = DMA_DEFAULT_TRIGGER_SOURCE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> DMA CHANNEL METHODS
///////////////////////////////////////////////////////////////////////////////////////////////////

DMAChannel::DMAChannel(int16_t channelIndex) : channelIndex(channelIndex) {
  allocated = false;
  ownerID = -1;
}


bool DMAChannel::setDescriptors(TransferDescriptor **descriptorArray, int16_t count, 
  bool loop, bool allocateDescriptors, bool preserveCurrentIndex) {
  // Check for exceptions
  DMA_STATUS prevStatus = getStatus();
  int16_t prevWbIndex = getWritebackIndex();
  if (descriptorArray == nullptr) return false;
  for (int16_t i = 0; i < count; i++) {
    if (descriptorArray[i] == nullptr) return false;
  }
  // current descriptor will actually be the previous one.
  if (descriptorCount > 0) clearDescriptors();
  DmacDescriptor *currentDescriptor = nullptr;

  // Copy first given descriptor into primariy descriptor array
  memcpy(&primaryDescriptorArray[channelIndex], &descriptorArray[0]->desc, 
    sizeof(DmacDescriptor));
  currentDescriptor = &primaryDescriptorArray[channelIndex];

  if (count > 1) {

    // Iterate through given descriptors & create/link them
    for (int16_t i = 1; i < count; i++) {
      DmacDescriptor *newDescriptor = nullptr;
      
      // Do we need to create new descriptors?
      if (allocateDescriptors) {
        newDescriptor = new DmacDescriptor;
        memcpy(newDescriptor, &descriptorArray[i]->desc, sizeof(DmacDescriptor));
      } else {
        newDescriptor = &descriptorArray[i]->desc;
      }
       
      // Link descriptor to next one in array
      currentDescriptor->DESCADDR.bit.DESCADDR = (uint32_t)newDescriptor;
      currentDescriptor = newDescriptor;

      // If preserve writeback = true -> link writeback to new descriptor @ current index
      if (preserveCurrentIndex && i == prevWbIndex 
      && prevStatus != DMA_CHANNEL_ERROR && prevStatus != DMA_CHANNEL_DISABLED
      && writebackDescriptorArray[channelIndex].SRCADDR.bit.SRCADDR | DMAC_SRCADDR_RESETVALUE
        != DMAC_SRCADDR_RESETVALUE) {
        writebackDescriptorArray[channelIndex].DESCADDR.bit.DESCADDR 
          = (uint32_t)currentDescriptor;
      }

    }
  }

  // Link the "current descriptor" to the first one if loop is true
  if (loop) {
    currentDescriptor->DESCADDR.bit.DESCADDR = (uint32_t)&primaryDescriptorArray[channelIndex];
  }
  descriptorCount = count;
  return true;
}


bool DMAChannel::setDescriptor(TransferDescriptor *descriptor, bool loop) {
  TransferDescriptor *descriptorArr[] = { descriptor };
  return setDescriptors(descriptorArr, 1, loop, false, false);
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


bool DMAChannel::queue(int16_t descriptorIndex) {
  CLAMP(descriptorIndex, 0, descriptorCount - 1);
  if (getStatus() == DMA_CHANNEL_DISABLED
  || getStatus() == DMA_CHANNEL_ERROR) {
    return false;
  }
  // If no writeback descriptor -> copy in primary descriptor
  if (writebackDescriptorArray[channelIndex].SRCADDR.bit.SRCADDR | DMAC_SRCADDR_RESETVALUE 
    == DMAC_SRCADDR_RESETVALUE) {
    memcpy(&writebackDescriptorArray[channelIndex], &primaryDescriptorArray[channelIndex],
      sizeof(DmacDescriptor));
  }
  // Find descriptor @ specified index
  DmacDescriptor *currentDescriptor = &primaryDescriptorArray[channelIndex]; 
  for (int16_t i = 0; i < descriptorIndex; i++) {
    currentDescriptor = (DmacDescriptor*)currentDescriptor->DESCADDR.bit.DESCADDR;
  }
  // Link writeback descriptor -> descriptor @ index (current descriptor)
  writebackDescriptorArray[channelIndex].DESCADDR.bit.DESCADDR = (uint32_t)currentDescriptor;
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


void DMAChannel::enableTrigger() {
  DMAC->Channel[channelIndex].CHCTRLA.bit.TRIGSRC = (uint8_t)externalTrigger;
  externalTriggerEnabled = true;
}

void DMAChannel::disableTrigger() {
  DMAC->Channel[channelIndex].CHCTRLA.bit.TRIGSRC = (uint8_t)TRIGGER_SOFTWARE;
  externalTriggerEnabled = false;
}


bool DMAChannel::reset(bool blocking) {
  if (!DMAC->Channel[channelIndex].CHCTRLA.bit.ENABLE) {
    return false;
  }
  // Disable the channel/interrupts & clear interrupt flags
  DMAC->Channel[channelIndex].CHCTRLA.bit.ENABLE = 0;
  DMAC->Channel[channelIndex].CHINTENCLR.reg &= DMAC_CHINTENCLR_RESETVALUE;
  DMAC->Channel[channelIndex].CHINTFLAG.reg &= DMAC_CHINTFLAG_RESETVALUE;

  // If blocking -> wait for sync
  if (blocking) {
    while(DMAC->Channel[channelIndex].CHSTATUS.bit.BUSY 
    || DMAC->Channel[channelIndex].CHSTATUS.bit.PEND);
  }
  
  // Clear flags / errors
  swTriggerFlag = false;
  swPendFlag = false;
  transferErrorFlag = false;
  currentError = DMA_ERROR_NONE;

  // Re-enable channel
  DMAC->Channel[channelIndex].CHCTRLA.bit.ENABLE = 1;
  return true;
}


void DMAChannel::clear() {
  externalTrigger = TRIGGER_SOFTWARE;
  descriptorCount = 0;
  swPendFlag = false;
  swTriggerFlag = false;
  transferErrorFlag = false;
  suspendFlag = false;
  externalTriggerEnabled = false;
  currentDescriptor = 0;

  clearDescriptors();

  descriptorsAllocated = false;

  // Disable the channel and call a soft reset of all registers
  DMAC->Channel[channelIndex].CHCTRLA.bit.ENABLE = 0;
  while(DMAC->Channel[channelIndex].CHCTRLA.bit.ENABLE);
  DMAC->Channel[channelIndex].CHCTRLA.bit.SWRST = 1;
  while(DMAC->Channel[channelIndex].CHCTRLA.bit.SWRST);
}


bool DMAChannel::isPending() { return getPending() > 0; }

bool DMAChannel::getEnabled() { return DMAC->Channel[channelIndex].CHCTRLA.bit.ENABLE; }


bool DMAChannel::isBusy() { 
return (DMAC->Channel[channelIndex].CHSTATUS.bit.BUSY 
    || DMAC->Channel[channelIndex].CHSTATUS.bit.PEND);
}


int16_t DMAChannel::getWritebackIndex() {
  if (getStatus() == DMA_CHANNEL_BUSY) {
    if (currentDescriptor + 1 == descriptorCount) {
      return 0;
    } else {
      return currentDescriptor + 1;
    }
  } else {
    return currentDescriptor;
  }
}


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

  } else if (suspendFlag || DMAC->Channel[channelIndex].CHCTRLB.bit.CMD 
    == DMAC_CHCTRLB_CMD_SUSPEND_Val) {
    return DMA_CHANNEL_SUSPENDED;

  } else if (DMAC->Channel[channelIndex].CHCTRLA.bit.ENABLE) {
    return DMA_CHANNEL_IDLE;

  } else {
    return DMA_CHANNEL_DISABLED;
  }
}

int16_t DMAChannel::getOwnerID() { return ownerID; }


void DMAChannel::init(int16_t ownerID) {
  // Disable & software reset channel
  DMAC->Channel[channelIndex].CHCTRLA.bit.ENABLE = 0;
  DMAC->Channel[channelIndex].CHCTRLA.bit.SWRST = 1;
  while(DMAC->Channel[channelIndex].CHCTRLA.bit.SWRST);

  settings.setDefault();

  currentError = DMA_ERROR_NONE;
  swPendFlag = false;
  swTriggerFlag = false;
  transferErrorFlag = false;
  suspendFlag = false;
  currentDescriptor = 0;
  descriptorsAllocated = false;
  descriptorCount = 0;
  externalTriggerEnabled = false;

  this->ownerID = ownerID;

  // Enable all interrupts
  DMAC->Channel[channelIndex].CHINTENSET.reg |= DMAC_CHINTENSET_MASK;
}


void DMAChannel::clearDescriptors() { ///// NEEDS TO BE UPDATED SO IT CONSIDERS IF DESCRIPTORS WERE ALLOCATED OR NOT...
  if (descriptorCount > 1) {
    DmacDescriptor *currentDescriptor 
      = (DmacDescriptor*)primaryDescriptorArray[channelIndex].DESCADDR.bit.DESCADDR;
    memset(&primaryDescriptorArray[channelIndex], 0, sizeof(DmacDescriptor));

      for (int16_t i = 0; i < descriptorCount - 1; i++) {
        DmacDescriptor *nextDescriptor
          = (DmacDescriptor*)primaryDescriptorArray[channelIndex].DESCADDR.bit.DESCADDR;

        // Was descriptor allocated by us?
        if (descriptorsAllocated) {
          delete currentDescriptor;
        } else {
          currentDescriptor->DESCADDR.reg = ~DMAC_DESCADDR_MASK;
        }
        // Move on to next descriptor
        currentDescriptor = nextDescriptor;
      }
      // Was descriptor allocated by us?
      if (descriptorsAllocated) {
        delete currentDescriptor;
      } else {
        currentDescriptor->DESCADDR.reg = ~DMAC_DESCADDR_MASK;
      }
      currentDescriptor = nullptr;
  
  } else if (descriptorCount == 1) {
    memset(&primaryDescriptorArray[channelIndex], 0, sizeof(DmacDescriptor));
  }
}
