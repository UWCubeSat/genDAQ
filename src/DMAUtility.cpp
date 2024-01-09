
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
  currentDesc = &desc;
  baseSourceAddr = 0;
  baseDestAddr = 0;
  primaryBound = false;
  validCount = 0;
  setDefault();
}

TransferDescriptor &TransferDescriptor::setSource(uint32_t sourceAddr,
  bool correctAddress = true) {
  if (sourceAddr == 0) return *this;
  if (correctAddress) {
    baseSourceAddr = sourceAddr;
    if (currentDesc->BTCTRL.bit.STEPSEL && currentDesc->BTCTRL.bit.SRCINC) {
      currentDesc->SRCADDR.bit.SRCADDR = sourceAddr + (currentDesc->BTCNT.bit.BTCNT 
        * (currentDesc->BTCTRL.bit.BEATSIZE + 1) 
        * (1 << currentDesc->BTCTRL.bit.STEPSIZE)); 
    } else {
      currentDesc->SRCADDR.bit.SRCADDR = sourceAddr + (currentDesc->BTCNT.bit.BTCNT
        * (currentDesc->BTCTRL.bit.BEATSIZE + 1));
    }   
  } else {
    currentDesc->SRCADDR.bit.SRCADDR = sourceAddr;
  }
  validCount++;
  if (validCount == 3) currentDesc->BTCTRL.bit.VALID = 1;
  return *this;
}

TransferDescriptor &TransferDescriptor::setDestination(uint32_t destinationAddr, 
  bool correctAddress = true) {
  if (destinationAddr == 0) return *this;
  if (correctAddress) {
    baseDestAddr = destinationAddr;
    if (!currentDesc->BTCTRL.bit.STEPSEL && currentDesc->BTCTRL.bit.DSTINC) {
      currentDesc->DSTADDR.bit.DSTADDR = destinationAddr + (currentDesc->BTCNT.bit.BTCNT
        * (currentDesc->BTCTRL.bit.BEATSIZE + 1)
        * (1 << currentDesc->BTCTRL.bit.STEPSIZE));
    } else {
      currentDesc->DSTADDR.bit.DSTADDR = destinationAddr + (currentDesc->BTCNT.bit.BTCNT
        * (currentDesc->BTCTRL.bit.BEATSIZE + 1));
    }
  } else {
    currentDesc->DSTADDR.bit.DSTADDR = destinationAddr;
  }
  validCount++;
  if (validCount == 3) currentDesc->BTCTRL.bit.VALID = 1;
  return *this;
}

TransferDescriptor &TransferDescriptor::setTransferAmount(uint16_t byteCount) {
  currentDesc->BTCNT.bit.BTCNT = (uint16_t)byteCount;
  if (baseDestAddr != 0) setDestination(baseDestAddr, true);
  if (baseSourceAddr != 0) setSource(baseSourceAddr, true);
  return *this;
}

TransferDescriptor &TransferDescriptor::setDataSize(int16_t byteCount) {
  if (byteCount == 1) {
    currentDesc->BTCTRL.bit.BEATSIZE = DMAC_BTCTRL_BEATSIZE_BYTE_Val;
  } else if (byteCount == 2) {
    currentDesc->BTCTRL.bit.BEATSIZE = DMAC_BTCTRL_BEATSIZE_HWORD_Val;
  } else if (byteCount == 4) {
    currentDesc->BTCTRL.bit.BEATSIZE = DMAC_BTCTRL_BEATSIZE_WORD_Val;
  } else {
    currentDesc->BTCTRL.bit.BEATSIZE = DMA_DEFAULT_DATA_SIZE;
  }
  validCount++;
  if (validCount == 3) currentDesc->BTCTRL.bit.VALID = 1;

  if (baseDestAddr != 0) setDestination(baseDestAddr, true);
  if (baseSourceAddr != 0) setSource(baseSourceAddr, true);
  return *this;
}

TransferDescriptor &TransferDescriptor::setIncrementConfig(bool incrementSource, bool incrementDestination) {
  if (incrementSource) {
    currentDesc->BTCTRL.bit.SRCINC = 1;
  } else {
    currentDesc->BTCTRL.bit.SRCINC = 0;
  }
  if (incrementDestination) {
    currentDesc->BTCTRL.bit.DSTINC = 1;
  } else {
    currentDesc->BTCTRL.bit.DSTINC = 0;
  }
  if (baseDestAddr != 0) setDestination(baseDestAddr, true);
  if (baseSourceAddr != 0) setSource(baseSourceAddr, true);
  return *this;
}

TransferDescriptor &TransferDescriptor::setIncrementModifier(DMA_TARGET target, int16_t modifier) {
  if (modifier > DMAC_BTCTRL_STEPSIZE_X128_Val || modifier < DMAC_BTCTRL_STEPSIZE_X1_Val) {
    currentDesc->BTCTRL.bit.STEPSIZE = DMA_DEFAULT_STEPSIZE;
    currentDesc->BTCTRL.bit.STEPSEL = DMA_DEFAULT_STEPSELECTION;
  } else {
    if (target = SOURCE) {
      currentDesc->BTCTRL.bit.STEPSEL = DMAC_BTCTRL_STEPSEL_SRC_Val;
    } else {
      currentDesc->BTCTRL.bit.STEPSEL = DMAC_BTCTRL_STEPSEL_DST_Val;
    }
    currentDesc->BTCTRL.bit.STEPSIZE = modifier;
  }
  if (baseDestAddr != 0) setDestination(baseDestAddr, true);
  if (baseSourceAddr != 0) setSource(baseSourceAddr, true);
  return *this;
}

TransferDescriptor &TransferDescriptor::setAction(DMA_TRANSFER_ACTION action) {
  currentDesc->BTCTRL.bit.BLOCKACT = (uint8_t)action;
}

void TransferDescriptor::setDefault() {
  currentDesc->BTCNT.bit.BTCNT = DMA_DEFAULT_TRANSFER_AMOUNT;
  currentDesc->DSTADDR.bit.DSTADDR = DMA_DEFAULT_DESTINATION;
  currentDesc->SRCADDR.bit.SRCADDR = DMA_DEFAULT_SOURCE;
  currentDesc->BTCTRL.bit.BEATSIZE = DMA_DEFAULT_DATA_SIZE;
  currentDesc->BTCTRL.bit.BLOCKACT = DMA_DEFAULT_TRANSFER_ACTION;
  currentDesc->BTCTRL.bit.DSTINC = DMA_DEFAULT_INCREMENT_DESTINATION;
  currentDesc->BTCTRL.bit.SRCINC = DMA_DEFAULT_INCREMENT_SOURCE;
  currentDesc->BTCTRL.bit.STEPSEL = DMA_DEFAULT_STEPSELECTION;
  currentDesc->BTCTRL.bit.STEPSIZE = DMA_DEFAULT_STEPSIZE;
  currentDesc->BTCTRL.bit.VALID = 0;
  baseSourceAddr = 0;
  baseDestAddr = 0;
  validCount = 0;
}

// Link is never unbound -> would take too much mem to save all linked descriptors
DmacDescriptor *TransferDescriptor::bindLink() { return &desc; }

void TransferDescriptor::bindPrimary(DmacDescriptor *primaryDescriptor) {
  currentDesc = primaryDescriptor;
  primaryBound = true;
}

void TransferDescriptor::unbindPrimary(DmacDescriptor *primaryDescriptor)  {
  memcpy(&desc, primaryDescriptor, sizeof(DmacDescriptor));
  primaryBound = false;
  currentDesc = &desc;
}

bool TransferDescriptor::isBindable() { return !primaryBound; }


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

DMAChannel::ChannelSettings &DMAChannel::ChannelSettings::setDescriptorsLooped(bool descriptorsLooped,
  bool updateWriteback) {
  
  // Are descriptors already looped?
  if (descriptorsLooped != super->descriptorsLooped) {
    if (descriptorsLooped) {
      super->loopDescriptors(updateWriteback);
    } else {
      super->unloopDescriptors(updateWriteback);
    } 
  }
  super->descriptorsLooped = descriptorsLooped;
  return *this;
}

void DMAChannel::ChannelSettings::removeCallbackFunction() {
  super->callback = nullptr;
}

DMAChannel::ChannelSettings &DMAChannel::ChannelSettings::setExternalTrigger(DMA_TRIGGER trigger) {
  super->externalTrigger = trigger;
  return *this;
}

void DMAChannel::ChannelSettings::removeExternalTrigger() {
  if (super->externalTriggerEnabled) {
    super->disableExternalTrigger();
  }
  super->externalTrigger = TRIGGER_SOFTWARE;
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
  if (super->descriptorsLooped && super->descriptorCount != 0) {
    super->unloopDescriptors(true);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> DMA CHANNEL METHODS
///////////////////////////////////////////////////////////////////////////////////////////////////

DMAChannel::DMAChannel(int16_t channelIndex) : channelIndex(channelIndex) {
  descriptorCount = 0;
  boundPrimary = nullptr;
  allocated = false;
  ownerID = -1;
  externalTriggerEnabled =false;
  currentError = DMA_ERROR_NONE;
  descriptorsLooped = false;
  swPendFlag = false;
  swTriggerFlag = false;
  transferErrorFlag = false;
  suspendFlag = false;
  currentDescriptor = 0;
  externalTrigger = TRIGGER_SOFTWARE;
  callback = nullptr;
  descriptorsLooped = false;
}


bool DMAChannel::setDescriptors(TransferDescriptor **descriptorArray, int16_t count, 
   bool bindDescriptors, bool updateWriteback) {
  DMA_STATUS prevStatus = getStatus();
  int16_t prevWbIndex = getWritebackIndex();

  // Check nullptr exceptions
  if (descriptorArray == nullptr) return false;
  for (int16_t i = 0; i < count; i++) {
    if (descriptorArray[i] == nullptr
    || (bindDescriptors && descriptorArray[i]->isBindable())) {
      return false;
    }
  }
  // current descriptor will actually be the previous one.
  if (descriptorCount > 0) clearDescriptors();
  DmacDescriptor *currentDescriptor = nullptr;

  // Copy first given descriptor into primariy descriptor array
  memcpy(&primaryDescriptorArray[channelIndex], &descriptorArray[0]->desc, 
    sizeof(DmacDescriptor));
  currentDescriptor = &primaryDescriptorArray[channelIndex];

  // Do we need to bind primary descriptor?
  if (bindDescriptors) {
    descriptorArray[0]->bindPrimary(&primaryDescriptorArray[channelIndex]);
    boundPrimary = descriptorArray[0];
  }

  // Do we need to link descriptors? -> Iterate through array if so
  if (count > 1) {
    for (int16_t i = 1; i < count; i++) {
      DmacDescriptor *newDescriptor = nullptr;

      // If binding descriptor -> dont alloc, otherwise -> alloc
      if (bindDescriptors) {
        newDescriptor = descriptorArray[i]->bindLink();
      } else {
        newDescriptor = new DmacDescriptor;
        memcpy(newDescriptor, &descriptorArray[i]->desc, sizeof(DmacDescriptor));
      }

      // Link descriptor to next one in array
      currentDescriptor->DESCADDR.bit.DESCADDR = (uint32_t)newDescriptor;
      currentDescriptor = newDescriptor;

      // If preserve writeback = true -> link writeback to new descriptor @ current index
      if (updateWriteback && i == prevWbIndex 
      && prevStatus != DMA_CHANNEL_ERROR && prevStatus != DMA_CHANNEL_DISABLED
      && writebackDescriptorArray[channelIndex].SRCADDR.bit.SRCADDR | DMAC_SRCADDR_RESETVALUE
        != DMAC_SRCADDR_RESETVALUE) {
        writebackDescriptorArray[channelIndex].DESCADDR.bit.DESCADDR 
          = (uint32_t)currentDescriptor;
      }

    }
  }
  // Link the "current descriptor" to the first one if loop is true
  if (descriptorsLooped) {
    currentDescriptor->DESCADDR.bit.DESCADDR 
      = (uint32_t)&primaryDescriptorArray[channelIndex];
  }
  descriptorCount = count;
  return true;
}


bool DMAChannel::setDescriptor(TransferDescriptor *descriptor, bool bindDescriptor) {
  TransferDescriptor *descriptorArr[] = { descriptor };
  return setDescriptors(descriptorArr, 1, bindDescriptor, false);
}


bool DMAChannel::replaceDescriptor(TransferDescriptor *updatedDescriptor, 
int16_t descriptorIndex, bool bindDescriptor) {

  // Check for exceptions
  CLAMP(descriptorIndex, 0, descriptorCount);
  if (descriptorCount == 0 || (bindDescriptor && updatedDescriptor->isBindable())) {
    return false;
  }
  // Handle cases -> descriptor is primary, or linked...
  if (descriptorIndex == 0) {
    // Unbind current descriptor
    if (boundPrimary != nullptr) {
      boundPrimary->unbindPrimary(&primaryDescriptorArray[channelIndex]);
      boundPrimary = nullptr;
    }
    // Copy new descriptor into primary slot
    memcpy(&primaryDescriptorArray[channelIndex], &updatedDescriptor->desc,
      sizeof(DmacDescriptor));

    // Bind the new primary descriptor (if applicable)
    if (bindDescriptor) {
      updatedDescriptor->bindPrimary(&primaryDescriptorArray[channelIndex]);
    }
  } else {
    DmacDescriptor *targetDescriptor = getDescriptor(descriptorIndex);
    uint32_t targetLink = targetDescriptor->DESCADDR.bit.DESCADDR;

    // If binding -> delete target & replace it with updated descriptor (get link), 
    //else -> copy into & overwrite current target with updated descriptor
    if (bindDescriptor) {
      delete targetDescriptor;
      targetDescriptor = updatedDescriptor->bindLink();
    } else {
      memcpy(targetDescriptor, &updatedDescriptor->desc, sizeof(DmacDescriptor));
    }
    // Link replaced descriptor to next (or not if no link)....
    targetDescriptor->DESCADDR.bit.DESCADDR = targetLink;
  }
  return true;
}


bool DMAChannel::addDescriptor(TransferDescriptor *descriptor, int16_t descriptorIndex, 
  bool updateWriteback, bool bindDescriptor = false) {

  DmacDescriptor *targetDescriptor = nullptr;
  DmacDescriptor *previousDescriptor = nullptr;
  DmacDescriptor *nextDescriptor = nullptr;

  // Handle exceptions
  DMA_STATUS currentStatus = getStatus();
  CLAMP(descriptorIndex, 0, descriptorCount);
  if (currentStatus == DMA_CHANNEL_ERROR
  || bindDescriptor && !descriptor->isBindable()) {
    return false;
  }
 
  // If no descriptors in list call set descriptor
  if (descriptorCount == 0) {
    setDescriptor(descriptor, bindDescriptor);
    return true;

  // Handle case: descriptor added to primary position
  } else if (descriptorIndex == 0) {

    // If descriptors looped get last descriptor
    if (descriptorsLooped) {
      previousDescriptor = getDescriptor(descriptorCount - 1);
    }

    // If primary bound, unbind & get new bound, linked descriptor
    if (boundPrimary != nullptr) {
      boundPrimary->unbindPrimary(&primaryDescriptorArray[channelIndex]);
      nextDescriptor = boundPrimary->bindLink();
      boundPrimary = nullptr;
    
    // Else -> Copy prev primary into new allocated descriptor
    } else {
      nextDescriptor = new DmacDescriptor();
      memcpy(nextDescriptor, &primaryDescriptorArray[channelIndex],
        sizeof(DmacDescriptor));
    }

    // Copy added descriptor into primary slot
    memcpy(&primaryDescriptorArray[channelIndex], descriptor->currentDesc,
      sizeof(DmacDescriptor));
    
    // If specified: bind added descriptor
    if (bindDescriptor) {
      descriptor->bindPrimary(&primaryDescriptorArray[channelIndex]);
      boundPrimary = descriptor;
    }
    targetDescriptor = &primaryDescriptorArray[channelIndex];

    // Link (added) primary descriptor to prev primary descriptor
    targetDescriptor->DESCADDR.bit.DESCADDR = (uint32_t)&nextDescriptor;

  // Handle case -> descriptor in middle of list or beyond end
  } else {
    previousDescriptor = getDescriptor(descriptorIndex - 1);
    
    // If bound -> dont alloc, Otherwise alloc new descriptor & copy into it
    if (bindDescriptor) {
      targetDescriptor = descriptor->bindLink();
    } else {
      targetDescriptor = new DmacDescriptor();
      memcpy(targetDescriptor, descriptor->currentDesc, sizeof(DmacDescriptor));
    }

    // If descriptor is being appended -> link only if list is looped
    if (descriptorIndex == descriptorCount && descriptorsLooped) {
      targetDescriptor->DESCADDR.bit.DESCADDR 
        = (uint32_t)&primaryDescriptorArray[channelIndex];
    
    // If not appended -> Link added descriptor to previous' next
    } else if (descriptorIndex != descriptorCount){
      targetDescriptor->DESCADDR.bit.DESCADDR 
        = previousDescriptor->DESCADDR.bit.DESCADDR; 
    }
    // Link previous descriptor to target (added one)
    previousDescriptor->DESCADDR.bit.DESCADDR = (uint32_t)targetDescriptor;
  } 

  // If wb linked to same descriptor as added one, it should be updated (if specified)
  if (writebackDescriptorArray[channelIndex].DESCADDR.bit.DESCADDR
    == targetDescriptor->DESCADDR.bit.DESCADDR) {
    
    writebackDescriptorArray[channelIndex].DESCADDR.bit.DESCADDR    
      = (uint32_t)targetDescriptor;
  }
  descriptorCount++;
  return true;
}


bool DMAChannel::removeDescriptor(int16_t descriptorIndex, bool updateWriteback) {
  // Handle exceptions
  DMA_STATUS currentStatus = getStatus();
  CLAMP(descriptorIndex, 0, descriptorCount - 1);
  if (descriptorCount == 0 || currentStatus == DMA_CHANNEL_ERROR) {
    return false;
  }

  DmacDescriptor *nextDescriptor = nullptr;
  DmacDescriptor *targetDescriptor = nullptr;
  DmacDescriptor *previousDescriptor = nullptr;
  uint32_t removedAddr = 0;

  // If removing last descriptor -> clear descriptors instead
  if (descriptorCount == 1) {
    clearDescriptors();
    return true;

  // Handle case -> Removing primary descriptor
  } else if (descriptorIndex == 0) {
    nextDescriptor = getDescriptor(1);
    removedAddr = (uint32_t)&primaryDescriptorArray[channelIndex];

    // Unbind primary descriptor (if applicable)
    if (boundPrimary != nullptr) {
      boundPrimary->unbindPrimary(&primaryDescriptorArray[channelIndex]);
    }

    // Move 2nd descriptor "down" by copying it into primary slot
    memcpy(&primaryDescriptorArray[channelIndex], nextDescriptor, 
      sizeof(DmacDescriptor));

    // If descriptors looped -> link last descriptor to new primary
    if (descriptorsLooped) {
      previousDescriptor = getDescriptor(descriptorCount - 1);
      previousDescriptor->DESCADDR.bit.DESCADDR 
        = (uint32_t)&primaryDescriptorArray[channelIndex];
    }

  // Else -> Removing either last or middle descriptor
  } else {

    // Get prev, targ & next if removing descriptor @ end of list.
    if (descriptorIndex = descriptorCount - 1) {
      previousDescriptor = getDescriptor(descriptorCount - 2);
      targetDescriptor = (DmacDescriptor*)previousDescriptor->DESCADDR.bit.DESCADDR;
      removedAddr = (uint32_t)targetDescriptor;
      
      // If looped -> get next (primary) descriptor, else -> leave next as nullptr
      if (descriptorsLooped) {  
        nextDescriptor = &primaryDescriptorArray[channelIndex];
      }

    // Get prev, targ & next if removing descriptor in middle of list.
    } else {
      previousDescriptor = getDescriptor(descriptorIndex);
      targetDescriptor = (DmacDescriptor*)previousDescriptor->DESCADDR.bit.DESCADDR;
      nextDescriptor = (DmacDescriptor*)targetDescriptor->DESCADDR.bit.DESCADDR;
      removedAddr = (uint32_t)targetDescriptor;
    }

    // Delete target descriptor
    delete targetDescriptor;
    targetDescriptor = nullptr;

    // If next descriptor is not nullptr -> link prev to it, else -> unlink prev
    if (nextDescriptor != nullptr) {
      previousDescriptor->DESCADDR.bit.DESCADDR = (uint32_t)nextDescriptor;
    } else {
      previousDescriptor->DESCADDR.bit.DESCADDR = 0;
    }
  }
  // Update writeback descriptor if it links to removed descriptor (if specified)
  if (updateWriteback && writebackDescriptorArray[channelIndex].DESCADDR.bit.DESCADDR 
    == removedAddr) {
    if (nextDescriptor == nullptr) {
      writebackDescriptorArray[channelIndex].DESCADDR.bit.DESCADDR = 0;
    } else {
      writebackDescriptorArray[channelIndex].DESCADDR.bit.DESCADDR
        = (uint32_t)nextDescriptor;
    }
  }
  descriptorCount--;
  return true;
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
  // Link writeback descriptor -> descriptor @ index (current descriptor)
  writebackDescriptorArray[channelIndex].DESCADDR.bit.DESCADDR 
    = (uint32_t)getDescriptor(descriptorIndex);
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


bool DMAChannel::enableExternalTrigger() {
  if (externalTrigger == TRIGGER_SOFTWARE) return false;
  DMAC->Channel[channelIndex].CHCTRLA.bit.TRIGSRC = (uint8_t)externalTrigger;
  externalTriggerEnabled = true;
}

bool DMAChannel::disableExternalTrigger() {
  if (!externalTriggerEnabled || externalTrigger == TRIGGER_SOFTWARE) return false;
  DMAC->Channel[channelIndex].CHCTRLA.bit.TRIGSRC = (uint8_t)TRIGGER_SOFTWARE;
  externalTriggerEnabled = false;
}


bool DMAChannel::setDescriptorValid(int16_t descriptorIndex, bool valid) {
  // Check for exceptions
  CLAMP(descriptorIndex, -1, descriptorCount);
  if (descriptorCount == 0) {
    return false;
  } else if (descriptorIndex == -1 && writebackDescriptorArray[channelIndex].SRCADDR.reg
    | DMAC_SRCADDR_RESETVALUE == 0) {
    return false;
  }
  
  if (descriptorIndex == -1) {
    // Validate writeback descriptor
    writebackDescriptorArray[channelIndex].BTCTRL.bit.VALID = (uint8_t)valid;

  } else { 
    // Validate the descriptor @ the specified index
    getDescriptor(descriptorIndex)->BTCTRL.bit.VALID = (uint8_t)valid;
  }
  return true;
} 


bool DMAChannel::getDescriptorValid(int16_t descriptorIndex) {
  // Check for exceptions
  CLAMP(descriptorIndex, -1, descriptorCount);
  if (primaryDescriptorArray[channelIndex].SRCADDR.reg | DMAC_SRCADDR_RESETVALUE == 0) {
    return false;
  } else if (descriptorIndex == -1 && writebackDescriptorArray[channelIndex].SRCADDR.reg
    | DMAC_SRCADDR_RESETVALUE == 0) {
    return false;
  }
  // Get the valid status of the requested descriptor
  if (descriptorIndex == -1) {
    return (bool)writebackDescriptorArray[channelIndex].BTCTRL.bit.VALID;

  } else { 
    return (bool)getDescriptor(descriptorIndex)->BTCTRL.bit.VALID;
  }
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

  boundPrimary = nullptr;
  descriptorsLooped = false;

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


bool DMAChannel::getExternalTriggerEnabled() {
  if (externalTrigger == TRIGGER_SOFTWARE) return false;
  return externalTriggerEnabled;
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


DmacDescriptor *DMAChannel::getDescriptor(int16_t descriptorIndex) {
  // If target descriptor cached -> use that one
  if (previousIndex == descriptorIndex) return previousDescriptor;

  // Else iterate through descriptors until at specified index
  DmacDescriptor *currentDescriptor = &primaryDescriptorArray[channelIndex];
  for (int16_t i = 1; i <= descriptorIndex; i++) {
    currentDescriptor = (DmacDescriptor*)currentDescriptor->DESCADDR.bit.DESCADDR;
  } 
  // Save descriptor & index to cache
  previousDescriptor = currentDescriptor;
  previousIndex = descriptorIndex;
  return currentDescriptor;
}


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
  descriptorCount = 0;
  externalTriggerEnabled = false;
  previousDescriptor = nullptr;

  this->ownerID = ownerID;

  // Enable all interrupts
  DMAC->Channel[channelIndex].CHINTENSET.reg |= DMAC_CHINTENSET_MASK;
}


void DMAChannel::clearDescriptors() { ///// NEEDS TO BE UPDATED SO IT NULLIFIES BOUND
  if (descriptorCount > 1) {
    // If primary is bound -> unbind it
    if (boundPrimary != nullptr) {
      boundPrimary->unbindPrimary(&primaryDescriptorArray[channelIndex]);
      boundPrimary = nullptr;
    }

    // Get descriptor linked to primary & clear primary using memset
    DmacDescriptor *currentDescriptor 
      = (DmacDescriptor*)primaryDescriptorArray[channelIndex].DESCADDR.bit.DESCADDR;
    memset(&primaryDescriptorArray[channelIndex], 0, sizeof(DmacDescriptor));

    for (int16_t i = 0; i < descriptorCount - 1; i++) {
      DmacDescriptor *nextDescriptor
        = (DmacDescriptor*)primaryDescriptorArray[channelIndex].DESCADDR.bit.DESCADDR;

        // Delete descriptor
        delete currentDescriptor;

      // Move on to next descriptor
      currentDescriptor = nextDescriptor;
    }
    delete currentDescriptor;
    currentDescriptor = nullptr;
  
  } else if (descriptorCount == 1) {
    memset(&primaryDescriptorArray[channelIndex], 0, sizeof(DmacDescriptor));
  }
  descriptorCount = 0;
}


void DMAChannel::loopDescriptors(bool updateWriteback) {
  DmacDescriptor *lastDescriptor = getDescriptor(descriptorCount - 1);

  // Is last descriptor already looped? -> If not link last -> primary (first)
  if (!descriptorsLooped) {
    descriptorsLooped = true;

    // Link last descriptor
    lastDescriptor->DESCADDR.bit.DESCADDR
      = (uint32_t)&primaryDescriptorArray[channelIndex];

    // Does the writeback descriptor also need to be looped? (if applicable)
    if (writebackDescriptorArray[channelIndex].DESCADDR.bit.DESCADDR == 0
    && updateWriteback) {
      writebackDescriptorArray[channelIndex].DESCADDR.bit.DESCADDR
        = (uint32_t)&primaryDescriptorArray[channelIndex];
    }
  }
} 


void DMAChannel::unloopDescriptors(bool updateWriteback) {
  DmacDescriptor *lastDescriptor = getDescriptor(descriptorCount - 1);

    // Ensure last descriptor looped before unlooping
  if (descriptorsLooped) {
    descriptorsLooped = false;

    // Unlink last descriptor
    lastDescriptor->DESCADDR.bit.DESCADDR = 0;

    // If writeback = last descriptor -> unloop it aswell (if specified)
    if (writebackDescriptorArray[channelIndex].DESCADDR.bit.DESCADDR
        == (uint32_t)&primaryDescriptorArray[channelIndex]
    && updateWriteback) {
      writebackDescriptorArray[channelIndex].DESCADDR.bit.DESCADDR = 0;
    }
  }
}
