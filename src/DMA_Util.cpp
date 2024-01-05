
//// PRE-PROCESSOR ////
#include <DMA_Util.h>
#include <malloc.h>
#define CH DMAC->Channel[channelIndex]

//// FORWARD DECLARATIONS ////
void DMAC_0_Handler(void);

void DMAC_1_Handler(void) __attribute__((weak, alias("DMAC_0_Handler")));  // Re-route all handlers to handler 0
void DMAC_2_Handler(void) __attribute__((weak, alias("DMAC_0_Handler")));
void DMAC_3_Handler(void) __attribute__((weak, alias("DMAC_0_Handler")));
void DMAC_4_Handler(void) __attribute__((weak, alias("DMAC_0_Handler")));

DMA_Utility &DMA;

//// COMMON VARIABLES ////
DmacDescriptor __attribute__((__aligned__(16)))
  primaryDescriptorArray[DMA_MAX_CHANNELS] SECTION_DMAC_DESCRIPTOR,    
  writebackArray[DMA_MAX_CHANNELS] SECTION_DMAC_DESCRIPTOR;

bool DMA_Utility::begun = false;

int16_t DMA_Utility::channelCount = 0;

DMAChannel DMA_Utility::channelArray[] = {
  DMAChannel(0), DMAChannel(1), DMAChannel(2), DMAChannel(3), DMAChannel(4),
  DMAChannel(5), DMAChannel(6), DMAChannel(7), DMAChannel(8), DMAChannel(9)
};

///////////////////////////////////////////////////////////////////////////////////////////////////

DMATask::DMATask() {
  resetSettings();
}

DMATask &DMATask::setIncrementConfig(bool incrementDestination, bool incrementSource) {   ///////////// CHANGE EVERYTHING TO HAVE SOURCE FIRST, DESTINATION SECOND
  if (incrementDestination) {
    taskDescriptor.BTCTRL.bit.DSTINC = 1;
  } else {
    taskDescriptor.BTCTRL.bit.DSTINC = 0;
  }
  if (incrementSource) {
    taskDescriptor.BTCTRL.bit.SRCINC = 1;
  } else {
    taskDescriptor.BTCTRL.bit.SRCINC = 0;
  }
  return *this;
}

DMATask &DMATask::setIncrementModifier(DMA_INCREMENT_MODIFIER modifier, DMA_TARGET target) {
  taskDescriptor.BTCTRL.bit.STEPSIZE = (uint8_t)modifier;
  taskDescriptor.BTCTRL.bit.STEPSEL = (uint8_t)target;
  return *this;
}

DMATask &DMATask::setElementSize(int16_t bytes) {
  if (bytes == 1) {
    taskDescriptor.BTCTRL.bit.BEATSIZE = DMAC_BTCTRL_BEATSIZE_BYTE_Val;
  } else if (bytes == 2) {
    taskDescriptor.BTCTRL.bit.BEATSIZE = DMAC_BTCTRL_BEATSIZE_HWORD_Val;
  } else if (bytes == 4) {
    taskDescriptor.BTCTRL.bit.BEATSIZE = DMAC_BTCTRL_BEATSIZE_WORD_Val;
  } else {
    taskDescriptor.BTCTRL.bit.BEATSIZE = DMA_DEFAULT_ELEMENT_SIZE;
  }
  return *this;
}

DMATask &DMATask::setTransferAmount(int16_t elements) {
  CLAMP(elements, 1, INT16_MAX);
  taskDescriptor.BTCNT.bit.BTCNT = elements;
  return *this;
}

DMATask &DMATask::setSource(void *sourcePointer) {
  if (sourcePointer == nullptr) {
    // => ASSERT GOES HERE
  } else {
    uint32_t startAddress = (uint32_t)sourcePointer;
    uint32_t targAddress = 0;
    if (taskDescriptor.BTCTRL.bit.STEPSEL && taskDescriptor.BTCTRL.bit.SRCINC) {
      targAddress = startAddress + (taskDescriptor.BTCNT.bit.BTCNT 
        * (taskDescriptor.BTCTRL.bit.BEATSIZE + 1) 
        * (1 << taskDescriptor.BTCTRL.bit.STEPSIZE)); 
    } else {
      targAddress = startAddress + (taskDescriptor.BTCNT.bit.BTCNT
        * (taskDescriptor.BTCTRL.bit.BEATSIZE + 1));
    }   
    taskDescriptor.SRCADDR.bit.SRCADDR = targAddress;
  }
  return *this;
}

DMATask &DMATask::setDestination(void *destinationPointer) {
  if (destinationPointer == nullptr) {
    // => ASSERT GOES HERE
  } else {
    uint32_t startAddress = (uint32_t)destinationPointer;
    uint32_t targAddress = 0;
    if (!taskDescriptor.BTCTRL.bit.STEPSEL && taskDescriptor.BTCTRL.bit.DSTINC) {
      targAddress = startAddress + (taskDescriptor.BTCNT.bit.BTCNT
        * (taskDescriptor.BTCTRL.bit.BEATSIZE + 1)
        * (1 << taskDescriptor.BTCTRL.bit.STEPSIZE));
    } else {
      targAddress = startAddress + (taskDescriptor.BTCNT.bit.BTCNT
        * (taskDescriptor.BTCTRL.bit.BEATSIZE + 1));
    }
    taskDescriptor.DSTADDR.bit.DSTADDR = targAddress;  
  }
  return *this;
}

void DMATask::resetSettings() {
  memset(&taskDescriptor, 0, sizeof(DmacDescriptor));
  taskDescriptor.BTCTRL.reg |= DMA_DEFAULT_DESCRIPTOR_MASK;
  taskDescriptor.BTCTRL.bit.VALID = 1;
  taskDescriptor.BTCNT.bit.BTCNT = DMA_DEFAULT_ELEMENT_TRANSFER_COUNT;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

DMASettings::DMASettings() {
  resetSettings();
}

DMASettings &DMASettings::setTransferThreshold(int16_t elements) {
  elements -= 1; 
  CLAMP(elements, 0, DMAC_CHCTRLA_THRESHOLD_8BEATS_Val);
  settingsMask &= ~DMAC_CHCTRLA_THRESHOLD_Msk;
  settingsMask |= DMAC_CHCTRLA_THRESHOLD(elements);
  return *this;
}

DMASettings &DMASettings::setBurstSize(int16_t elements) {
  elements -= 1;
  CLAMP(elements, 0, DMAC_CHCTRLA_BURSTLEN_16BEAT_Val);
  settingsMask &= ~DMAC_CHCTRLA_BURSTLEN_Msk;
  settingsMask |= DMAC_CHCTRLA_BURSTLEN((uint8_t)elements);
  return *this;
}

DMASettings &DMASettings::setTransferMode(DMA_TRANSFER_MODE transferMode) {
  settingsMask &= ~DMAC_CHCTRLA_TRIGACT_Msk;
  settingsMask |= DMAC_CHCTRLA_TRIGACT((uint8_t)transferMode);
  return *this;
}

DMASettings &DMASettings::setSleepConfig(bool enabledDurringSleep) {
  if (enabledDurringSleep) {
    settingsMask |= DMAC_CHCTRLA_RUNSTDBY;
  } else {
    settingsMask &= ~DMAC_CHCTRLA_RUNSTDBY;
  }
  return *this;
}

DMASettings &DMASettings::setPriorityLevel(DMA_PRIORITY_LEVEL level) {
  priorityLevel = (uint8_t)level;
  return *this;
}

DMASettings &DMASettings::setCallbackFunction(void (*callbackFunction)
  (DMA_INTERRUPT_REASON, int16_t, DMAChannel&)) {
  this->callbackFunction = callbackFunction;
  return *this;
}

void DMASettings::removeCallbackFunction() {
  callbackFunction = nullptr;
}

void DMASettings::resetSettings() {
  settingsMask &= ~DMAC_CHCTRLA_MASK;
  settingsMask |= DMAC_CHCTRLA_THRESHOLD(DMA_DEFAULT_TRANSMISSION_THRESHOLD);
  settingsMask |= DMAC_CHCTRLA_BURSTLEN(DMA_DEFAULT_BURST_SIZE);
  settingsMask |= DMAC_CHCTRLA_TRIGACT(DMA_DEFAULT_TRIGGER_ACTION);
  settingsMask |= DMAC_CHCTRLA_TRIGSRC(DMA_DEFAULT_TRIGGER_SOURCE);
  if (DMA_DEFAULT_SLEEP_CONFIG) {
    settingsMask |= DMAC_CHCTRLA_RUNSTDBY;
  }
  priorityLevel = 0;
  callbackFunction = nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void DMAC_0_Handler(void) { 
  // Get interrupt source & set up variables
  DMAChannel &interruptSource = DMA[DMAC->INTPEND.bit.ID];
  DMA_INTERRUPT_REASON callbackArg = REASON_UNKNOWN;
  DMA_TRIGGER_SOURCE interruptTrigger = TRIGGER_SOFTWARE;

  // Has there been a transfer error? -> Get type & set flag
  if (DMAC->INTPEND.bit.TERR) {
    if (DMAC->INTPEND.bit.CRCERR) {                           
      callbackArg = REASON_ERROR_CRC;
      interruptSource.channelError = CHANNEL_ERROR_CRC_ERROR;
    } else {
      callbackArg = REASON_ERROR_TRANSFER;
      interruptSource.channelError = CHANNEL_ERROR_TRANSFER_ERROR;
    }
  // Has the channel been suspended?
  } else if (DMAC->Channel[interruptSource.channelIndex].CHINTFLAG.bit.SUSP) {
    
    // Is it due to a descriptor error? -> Set flag
    if (DMAC->Channel[interruptSource.channelIndex].CHSTATUS.bit.FERR){
      interruptSource.channelError = CHANNEL_ERROR_DESCRIPTOR_ERROR;     
      callbackArg = REASON_ERROR_DESCRIPTOR;                            

    // Is it due to client pause command? -> Do nothing 
    } else if (DMAC->Channel[interruptSource.channelIndex].CHCTRLB.bit.CMD 
        == DMAC_CHCTRLB_CMD_SUSPEND_Val){ 
      callbackArg = REASON_UNKNOWN;

    // Is it due to a pause request on block? -> Un-suspend channel
    } else {
      callbackArg = REASON_TRANSFER_COMPLETE;

      // Unpause the channel (if applicable)
      if (interruptSource.autoPause) {
        interruptSource.pauseFlag = true;
      } else {
        DMAC->Channel[interruptSource.channelIndex].CHCTRLB.bit.CMD 
          = DMAC_CHCTRLB_CMD_RESUME_Val;
      }

      // Update the task counter: If current = primary -> reset it, else -> increment it
      updateTaskCounter: {
        if (writebackArray[interruptSource.channelIndex].DESCADDR.bit.DESCADDR
            == primaryDescriptorArray[interruptSource.channelIndex].DESCADDR.bit.DESCADDR) {
          interruptSource.activeTask = 0;
        } else {
          interruptSource.activeTask++;
        }
      }
    }
  }
  // Is the interrupt source a peripheral trigger?
  if (!interruptSource.swTriggerFlag) {
    interruptTrigger = (DMA_TRIGGER_SOURCE)
    DMAC->Channel[interruptSource.channelIndex].CHCTRLA.bit.TRIGSRC;
  }
  // Update the software trigger flags
  interruptSource.swTriggerFlag = interruptSource.swPendingFlag;
  interruptSource.swPendingFlag = false;

  // If callback exists & valid reason -> call callback
  if (interruptSource.callbackFunction != nullptr) {
    interruptSource.callbackFunction(callbackArg, interruptTrigger, interruptSource.activeTask, interruptSource);
  }
  // Clear all flags
  DMAC->INTPEND.bit.TCMPL = 1; 
  DMAC->INTPEND.bit.SUSP = 1;
  DMAC->INTPEND.bit.TERR = 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bool DMA_Utility::begin() { 
  end(); // Reset DMA  
  
  // Configure DMA settings
  MCLK->AHBMASK.bit.DMAC_ = 1;                                     // Enable DMA clock.
  DMAC->BASEADDR.bit.BASEADDR = (uint32_t)primaryDescriptorArray;  // Set primary descriptor storage SRAM address
  DMAC->WRBADDR.bit.WRBADDR = (uint32_t)writebackArray;            // Set writeback descriptor storage SRAM address              
  
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
  return true;
}


bool DMA_Utility::end() {
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
    channelArray[i].clearChannel();
  }
  // Reset variables
  channelCount = 0;
  begun = false;
  return true;
}


DMAChannel &DMA_Utility::getChannel(const int16_t channelIndex) {
  CLAMP(channelIndex, 0, DMA_MAX_CHANNELS - 1);
  return channelArray[channelIndex];
}


DMAChannel &DMA_Utility::operator[](const int16_t channelIndex) {
  return getChannel(channelIndex);
}


///////////////////////////////////////////////////////////////////////////////////////////////////

DMAChannel::DMAChannel(const int16_t channelIndex) 
  : channelIndex(channelIndex) {}


bool DMAChannel::setTasks(DMATask **tasks, int16_t numTasks) {  
  // Establish temp variables
  DmacDescriptor *currentDescriptor = &primaryDescriptorArray[channelIndex];; 
  bool prevBusy = false;            
  int16_t writebackIndex = -1;
  int16_t nullTasks = 0;

  // Check for exceptions
  DMA_CHANNEL_STATUS currentStatus = getStatus();
  if (tasks == nullptr 
  || numTasks > DMA_MAX_TASKS 
  || numTasks <= 0) { 
    return false;
  }
  // Count number of null tasks in passed array
  for (int i = 0; i < numTasks; i++) {
    if (tasks[i] == nullptr) {
      nullTasks++;
    }
  }
  // Handle null tasks
  if (nullTasks == numTasks) {
    clearTasks();
    return true;
  } else if (nullTasks != 0) {
    return false;
  }
  // Special case -> currently busy
  if (currentStatus == CHANNEL_STATUS_BUSY) {
    prevBusy = true;
    suspend();
  }
  // Remove current tasks
  clearTasks();  

  // Copy first descriptor into primary storage
  memcpy(&primaryDescriptorArray[channelIndex], &tasks[0]->taskDescriptor, 
    sizeof(DmacDescriptor));

  // If active handle re-linking written-back descriptor/task
  if (writebackArray[channelIndex].DESCADDR.bit.DESCADDR != 0) {

    // Handle case -> currently task is at end of list or is only task
    if (writebackArray[channelIndex].DESCADDR.bit.DESCADDR
        == (uint32_t)&primaryDescriptorArray[channelIndex]) {
      writebackIndex = getTaskCount() - 1;
    
    } else { // -> iterate through linked list of tasks/descriptors
      for (int16_t i = 1; i < getTaskCount() - 1; i++) {
        currentDescriptor = (DmacDescriptor*)currentDescriptor->DESCADDR.bit.DESCADDR;

        // Does currently executing task link to this index?
        if (writebackArray[channelIndex].DESCADDR.bit.DESCADDR 
            == (uint32_t)currentDescriptor) { 
          

          // Set link to start of new chain if not "in" chain
          if (i >= numTasks) {
            writebackArray[channelIndex].DESCADDR.bit.DESCADDR 
              = (uint32_t)&primaryDescriptorArray[channelIndex];
            writebackIndex = -1; // Invalidate wbi since alr linked...
          
          } else { // -> Set index variable for use in next step...
            writebackIndex = i - 1; 
          }
          break;
        }
      }
    }
  }
  // Link tasks together in chain
  if (numTasks > 1) {
    // Link primary task to next task -> begins linked list of tasks
    currentDescriptor = createDescriptor(&tasks[1]->taskDescriptor);
    primaryDescriptorArray[channelIndex].DESCADDR.bit.DESCADDR 
      = (uint32_t)currentDescriptor;

    // Link all other tasks in passed array (if applicable)
    for (int16_t i = 1; i < numTasks - 1 && i < DMA_MAX_TASKS - 1; i++) {
      DmacDescriptor *nextDescriptor = createDescriptor(&tasks[i + 1]->taskDescriptor); // Copy next descriptor
      currentDescriptor->DESCADDR.bit.DESCADDR = (uint32_t)nextDescriptor;              // Link current -> next
      currentDescriptor = nextDescriptor;                                               // Iterate "currentDescriptor"

      // If at index of executing descriptor link it
      if (i == writebackIndex + 1) {
        writebackArray[channelIndex].DESCADDR.bit.DESCADDR = (uint32_t)nextDescriptor;
      }
    }
  } else { // -> Link task to itself (loop)
    currentDescriptor = &primaryDescriptorArray[channelIndex];
  }
  // Link last descriptor (currentDescriptor) -> first (loop chain)
  currentDescriptor->DESCADDR.bit.DESCADDR 
    = (uint32_t)&primaryDescriptorArray[channelIndex];

  // Un-suspend channel (if applicable)
  if (prevBusy) {
    resume();
  }
  // Enable the channel & interrupts (if applicable)
  CH.CHCTRLA.bit.ENABLE = 1;
  if ((CH.CHINTENSET.reg & DMAC_CHINTENSET_MASK) != DMAC_CHINTENSET_MASK) {
    CH.CHINTENSET.reg |= DMAC_CHINTENSET_MASK;
  }
  return true;
}


bool DMAChannel::addTask(DMATask *task, int16_t taskIndex) {
  // Establish temp variables
  DMA_CHANNEL_STATUS currentStatus = getStatus();
  int16_t taskCount = getTaskCount();
  DmacDescriptor *previousDescriptor = &primaryDescriptorArray[channelIndex];
  DmacDescriptor *newDescriptor = nullptr;
  bool prevBusy = false;

  // Clamp taskIndex & check for status exception
  if (taskIndex == -1) {
    taskIndex = taskCount;
  }
  CLAMP(taskIndex, 0, taskCount);
  if (task == nullptr) {
    return false;
  }
  // Does channel need to be suspended?
  if (currentStatus == CHANNEL_STATUS_BUSY) {
    prevBusy = true;
    suspend();
  }
  // Inserting at beggining?
  if (taskIndex == 0) {

    // Does a primary descriptor alr exist?
    if (taskCount > 0) {
      newDescriptor = createDescriptor(&primaryDescriptorArray[channelIndex]); // New desciptor = primary descriptor
    } else {  
      newDescriptor = &primaryDescriptorArray[channelIndex]; // "New" descriptor = added descriptor
    }
    // Copy added descriptor into primary slot & link it to "new" dsecriptor
    memcpy(&primaryDescriptorArray[channelIndex], &task->taskDescriptor,  
      sizeof(DmacDescriptor));
    primaryDescriptorArray[channelIndex].DESCADDR.bit.DESCADDR = (uint32_t)newDescriptor;

  // Inserting at end? -> New descriptor = added descriptor
  } else if (taskIndex == getTaskCount()) {
    newDescriptor = createDescriptor(&task->taskDescriptor);

    // Get last descriptor in chain
    for (int16_t i = 0; i < getTaskCount() - 1; i++) {
      previousDescriptor = (DmacDescriptor*)previousDescriptor->DESCADDR.bit.DESCADDR;
    }
    // Link last -> new and new -> primary
    previousDescriptor->DESCADDR.bit.DESCADDR = (uint32_t)newDescriptor;
    newDescriptor->DESCADDR.bit.DESCADDR = (uint32_t)&primaryDescriptorArray[channelIndex];
  
  // Inserting in middle? -> New descriptor = added descriptor
  } else {
    newDescriptor = createDescriptor(&task->taskDescriptor);

    // Find previous descriptor
    for (int16_t i = 0; i < taskIndex - 1; i++) {
      previousDescriptor = (DmacDescriptor*)previousDescriptor->DESCADDR.bit.DESCADDR;
    }
    // Link new -> previous' old link & previous -> new 
    newDescriptor->DESCADDR.bit.DESCADDR = previousDescriptor->DESCADDR.bit.DESCADDR;
    previousDescriptor->DESCADDR.bit.DESCADDR = (uint32_t)newDescriptor;
  }
  // If active descriptor shares added descriptor's link -> it should now link to the added descriptor.
  if (writebackArray[channelIndex].DESCADDR.bit.DESCADDR
      == newDescriptor->DESCADDR.bit.DESCADDR) {
    writebackArray[channelIndex].DESCADDR.bit.DESCADDR = (uint32_t)newDescriptor;
  }
  // Un-suspend channel (if applicable)
  if (prevBusy) {
    resume();
  }
  // Enable the channel & interrupts (if applicable)
  CH.CHCTRLA.bit.ENABLE = 1;
  if ((CH.CHINTENSET.reg & DMAC_CHINTENSET_MASK) != DMAC_CHINTENSET_MASK) {
    CH.CHINTENSET.reg |= DMAC_CHINTENSET_MASK;
  }
  return true;
}


bool DMAChannel::removeTask(int16_t taskIndex) {  
  // Establish temp variables
  DMA_CHANNEL_STATUS currentStatus = getStatus();
  DmacDescriptor *currentDescriptor = nullptr;   
  uint32_t removedAddress = 0;
  uint32_t nextAddress = 0;
  bool prevBusy = false;

  // Clamp taskIndex & check for status exception
  CLAMP(taskIndex, 0, getTaskCount() - 1);

  // Removing last descriptor? -> clear tasks (disables the channel)
  if (primaryDescriptorArray[channelIndex].DESCADDR.bit.DESCADDR == 0) {
    clearTasks();

  // Else -> Ensure channel is paused
  } else if (currentStatus == CHANNEL_STATUS_BUSY) {
    suspend();
  }
  // Removing primary descriptor?
  if (taskIndex == 0) {
    removedAddress = (uint32_t)&primaryDescriptorArray[channelIndex];

    // Overwrite the primary descriptor with 2nd one & delete it (the 2nd one).
    currentDescriptor = (DmacDescriptor*)primaryDescriptorArray[channelIndex] 
      .DESCADDR.bit.DESCADDR;
    nextAddress = (uint32_t)currentDescriptor;
    memcpy(&primaryDescriptorArray[channelIndex], currentDescriptor,         
      sizeof(DmacDescriptor));    
    delete currentDescriptor;          

  // Handle cases -> removing from middle/end of chain
  } else {
    currentDescriptor = &primaryDescriptorArray[channelIndex];
    DmacDescriptor *previousDescriptor = nullptr;

    // Iterate through descriptors until currentDescriptor = target
    for (int16_t i = 0; i < taskIndex; i++) {
      previousDescriptor = currentDescriptor;
      currentDescriptor = (DmacDescriptor*)currentDescriptor->DESCADDR.bit.DESCADDR;
    }
    // Save addresses & unlink/delete target descriptor
    removedAddress = (uint32_t)currentDescriptor;
    nextAddress = currentDescriptor->DESCADDR.bit.DESCADDR;
    previousDescriptor->DESCADDR.bit.DESCADDR = nextAddress;
    delete currentDescriptor;
  }
  // Update currently executing descriptor (if applicable)
  if (currentStatus == CHANNEL_STATUS_PAUSED
  && writebackArray[channelIndex].DESCADDR.bit.DESCADDR == removedAddress) {
    writebackArray[channelIndex].DESCADDR.bit.DESCADDR = nextAddress;
  }
  // Un-suspend channel (if applicable)
  if (prevBusy) {
    resume();
  }
  return true;
}


bool DMAChannel::clearTasks() {  
  // Establish temp variables
  uint32_t startAddress = (uint32_t)&primaryDescriptorArray[channelIndex];                                           
  DmacDescriptor *currentDescriptor = nullptr;
  DmacDescriptor *nextDescriptor = nullptr;

  // Check exceptions                                                                 
  if (primaryDescriptorArray[channelIndex].SRCADDR.bit.SRCADDR == 0) {
    return true;
  }
  // Disable the chnanel
  CH.CHCTRLA.bit.ENABLE = 0;

  // Are there linked descriptors? If so, get the first one
  if (primaryDescriptorArray[channelIndex].DESCADDR.bit.DESCADDR != startAddress) {
    currentDescriptor 
      = (DmacDescriptor*)primaryDescriptorArray[channelIndex].DESCADDR.bit.DESCADDR;

    // Now, iterate through & delete linked descriptors until we loop back to start
    while((uint32_t)currentDescriptor != startAddress) {
      nextDescriptor = (DmacDescriptor*)currentDescriptor->DESCADDR.bit.DESCADDR;
      delete currentDescriptor;
      currentDescriptor = nextDescriptor;
    }
  }
  // Can't delete primary/writeback descriptor so clear it...
  memset(&primaryDescriptorArray[channelIndex], 0, sizeof(DmacDescriptor));
  memset(&writebackArray[channelIndex], 0, sizeof(DmacDescriptor));
  return true;
}


int16_t DMAChannel::getTaskCount() {
  // Get primary descriptor. & create temp/count variables
  DmacDescriptor *currentDescriptor = 
  &primaryDescriptorArray[channelIndex];
  uint32_t firstDescriptor = (uint32_t)currentDescriptor;
  int16_t currentDescriptorCount = 0;
  
  // If primary descriptor is empty -> 0 tasks
  if (currentDescriptor->SRCADDR.bit.SRCADDR == 0) {
    return 0;
                                                                                                                                                
  // More then one descriptor -> iterate until linked = first descriptor
  } else {
    currentDescriptorCount = 1;
    while(currentDescriptor->DESCADDR.reg != firstDescriptor) {
      currentDescriptor = (DmacDescriptor*)currentDescriptor->DESCADDR.reg;
      currentDescriptorCount++;
    }
    return currentDescriptorCount;
  }
}

bool DMAChannel::trigger() {
  // Handle exceptions
  DMA_CHANNEL_STATUS currentStatus = getStatus();
  if (currentStatus == CHANNEL_STATUS_NULL
  || currentStatus == CHANNEL_STATUS_ERROR) {
    return false;
  }
  // Enable channel & interrupts (if applicable)
  if (!CH.CHCTRLA.bit.ENABLE) CH.CHCTRLA.bit.ENABLE = 1;
  if ((CH.CHINTENSET.reg & DMAC_CHINTENSET_MASK) != DMAC_CHINTENSET_MASK) {
    CH.CHINTENSET.reg |= DMAC_CHINTENSET_MASK;
  }
  // Trigger channel -> Or set pend flag
  if (CH.CHSTATUS.bit.PEND) {
    swPendingFlag = true;
  } else {
    swTriggerFlag = true;
    DMAC->SWTRIGCTRL.reg |= (1 << channelIndex);
  }
  return true;
}


bool DMAChannel::reset() {
  // Handle exceptions
  DMA_CHANNEL_STATUS currentStatus = getStatus();
  if (currentStatus == CHANNEL_STATUS_NULL
  || currentStatus == CHANNEL_STATUS_ERROR) {
    return false;
  }
  // Reset channel & flags
  CH.CHCTRLA.bit.ENABLE = 0;
  CH.CHCTRLA.bit.ENABLE = 1;
  swTriggerFlag = false;
  swPendingFlag = false;

  // Restore previous state
  if (currentStatus == CHANNEL_STATUS_PAUSED) {
    CH.CHCTRLB.bit.CMD = DMAC_CHCTRLB_CMD_SUSPEND_Val;
  }
  return true;
}


bool DMAChannel::suspend() {
  // Handle exceptions
  DMA_CHANNEL_STATUS currentStatus = getStatus();
  if (currentStatus == CHANNEL_STATUS_NULL
  || currentStatus == CHANNEL_STATUS_ERROR) {
    return false;
  }
  // Send pause command
  CH.CHCTRLB.bit.CMD = DMAC_CHCTRLB_CMD_SUSPEND_Val;
  pauseFlag = true;
  return true;
}


bool DMAChannel::resume() {
  // Handle exceptions
  DMA_CHANNEL_STATUS currentStatus = getStatus();
  if (currentStatus == CHANNEL_STATUS_NULL
  || currentStatus == CHANNEL_STATUS_ERROR) {
    return false;
  }
  // Send resume command
  CH.CHCTRLB.bit.CMD = DMAC_CHCTRLB_CMD_RESUME_Val;
  pauseFlag = false;
  return true;
}


bool DMAChannel::jump(int16_t index) {
  // Handle exceptions
  DMA_CHANNEL_STATUS currentStatus = getStatus(); 
  CLAMP(index, 0, getTaskCount());
  if (currentStatus == CHANNEL_STATUS_NULL
  || currentStatus == CHANNEL_STATUS_ERROR) {
    return false;
  }
  // Change link of writeback descriptor (if applicable)
  if (writebackArray[channelIndex].SRCADDR.bit.SRCADDR != DMAC_SRCADDR_RESETVALUE) {
    DmacDescriptor *currentDescriptor = &primaryDescriptorArray[channelIndex];
    for (int16_t i = 0; i < index; i++) {
      currentDescriptor = (DmacDescriptor*)currentDescriptor->DESCADDR.bit.DESCADDR;
    }
    writebackArray[channelIndex].DESCADDR.bit.DESCADDR = (uint32_t)currentDescriptor;
  } else {
    return false;
  }
  return true;
}


bool DMAChannel::enableExternalTrigger(DMA_TRIGGER_SOURCE source) {
  // Handle exceptions
  if (getStatus() == CHANNEL_STATUS_BUSY) {
    return false;
  }
  // Capture initial status
  bool wasEnabled = false;
  bool wasPaused = false;
  int16_t initIndex = 9;
  if (CH.CHCTRLA.bit.ENABLE) {
    wasEnabled = true;
  }
  if (pauseFlag) {
    wasPaused = true;
  }
  if (primaryDescriptorArray[channelIndex].SRCADDR.bit.SRCADDR != DMAC_SRCADDR_RESETVALUE) {
    initIndex = activeTask;
  }
  // Set up external trigger
  CH.CHCTRLA.bit.ENABLE = 0;                // Disable channel
  while(CH.CHCTRLA.bit.ENABLE);             // Sync
  CH.CHCTRLA.bit.TRIGSRC = (uint8_t)source; // Set trigger source

  // Update flags
  swPendingFlag = false;
  swTriggerFlag = false;

  // Restore initial state
  if (wasEnabled) {
    CH.CHCTRLA.bit.ENABLE = 1;
  }
  if (wasPaused) {
    CH.CHCTRLB.bit.CMD = DMAC_CHCTRLB_CMD_SUSPEND_Val;
  }
  if (initIndex > 0) { // This is really the only solution...
    jump(initIndex);
  }
  return true;
}


bool DMAChannel::disableExternalTrigger() {
  // Handle exceptions
  if (CH.CHCTRLA.bit.TRIGACT == TRIGGER_SOFTWARE) {
    return true;
  } else if (getStatus() == CHANNEL_STATUS_BUSY) {
    return false;
  }
  // Capture previous state
  bool wasEnabled = false;
  bool wasPaused = false;
  int16_t initIndex = 0;
  if (CH.CHCTRLA.bit.ENABLE) {
    wasEnabled = true;
    if (pauseFlag) {
      wasPaused = true;
    }
    if (primaryDescriptorArray[channelIndex].SRCADDR.bit.SRCADDR
        != DMAC_SRCADDR_RESETVALUE) {
      initIndex = activeTask;
    }
  }
  // Disable channel and reset trigger
  CH.CHCTRLA.bit.ENABLE = 0;                       
  while(CH.CHCTRLA.bit.ENABLE == 1);               
  CH.CHCTRLA.bit.TRIGSRC = (uint8_t)DMA_DEFAULT_TRIGGER_SOURCE;       
  CH.CHCTRLA.bit.TRIGACT = (uint8_t)DMA_DEFAULT_TRIGGER_ACTION;        
  CH.CHCTRLA.bit.ENABLE = 1; 

  // Update flags
  swPendingFlag = false;
  swTriggerFlag = false;

  // Restore intial state
  if (wasEnabled) {
    CH.CHCTRLA.bit.ENABLE = 1;
    if (wasPaused) {
      CH.CHCTRLB.bit.CMD = DMAC_CHCTRLB_CMD_SUSPEND_Val;
    }
    if (initIndex > 0) {
      jump(initIndex);
    }
  }

  return true;
}


bool DMAChannel::changeExternalTrigger(DMA_TRIGGER_SOURCE newSource) {
  return enableExternalTrigger(newSource);
}


void DMAChannel::clearChannel() {
  clearTasks();
  resetFields();

  // Disable & reset channel
  CH.CHCTRLA.bit.ENABLE = 0;
  CH.CHCTRLA.bit.SWRST = 1;   
}


DMA_CHANNEL_STATUS DMAChannel::getStatus() {
  if (channelError != CHANNEL_ERROR_NONE) {
    return CHANNEL_STATUS_ERROR;
  } else if (CH.CHSTATUS.bit.BUSY) {  
    return CHANNEL_STATUS_BUSY;
  } else if (CH.CHSTATUS.bit.PEND) {
    return CHANNEL_STATUS_BUSY;
  } else if (pauseFlag) {
    return CHANNEL_STATUS_PAUSED;
  } else if (primaryDescriptorArray[channelIndex].SRCADDR.bit.SRCADDR
      != DMAC_SRCADDR_RESETVALUE) {
    return CHANNEL_STATUS_IDLE;
  } else {
    return CHANNEL_STATUS_NULL;
  }
}

int16_t DMAChannel::getPending() {
  // Is trigger pending?
  if (CH.CHSTATUS.bit.PEND) {
    // What trigger is it?
    if (swPendingFlag) {
      return 1;
    } else {
      return 2;
    }
  } else {
    return 0;
  }
}

bool DMAChannel::isBusy() {
  return getStatus() == CHANNEL_STATUS_BUSY;
}

int16_t DMAChannel::getActiveTask() {
  return activeTask;
}

DMA_CHANNEL_ERROR DMAChannel::getError() {
  DMA_CHANNEL_ERROR currentError = channelError;
  return currentError;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void DMAChannel::resetFields() {
  channelError = CHANNEL_ERROR_NONE;
  activeTask = 0;
  swTriggerFlag = false;
  swPendingFlag = false;
  autoPause = false;
  pauseFlag = false;
  callbackFunction = nullptr;
}

DmacDescriptor *DMAChannel::createDescriptor(const DmacDescriptor *referenceDescriptor) {
  // Create descriptor in SRAM -> 128 bit aligned
  DmacDescriptor *newDescriptor = (DmacDescriptor*)memalign(16, sizeof(DmacDescriptor)); 

  // Copy values from reference into new descriptor.
  if (referenceDescriptor != nullptr) {                           
    memcpy(newDescriptor, referenceDescriptor, sizeof(DmacDescriptor)); 
  }                                                                                           
  return newDescriptor;  
}


void printReg32(uint32_t reg, uint32_t mask) {
  for (int i = 0; i < 32; i++) {
    if (((mask >> i) & 1) == 1) {
      Serial.print((String)((reg >> i) & 1));
    } else {
      Serial.print("#");
    }
  }
  Serial.println("");
}

void printReg16(uint16_t reg, uint16_t mask) {
  for (int i = 0; i < 16; i++) {
    if (((mask >> i) & 1) == 1) {
      Serial.print((String)((reg >> i) & 1));
    } else {
      Serial.print("#");
    }
  }
  Serial.println("");
}

void printReg8(uint8_t reg, uint8_t mask) {
  for (int i = 0; i < 8; i++) {
    if (((mask >> i) & 1) == 1) {
      Serial.print((String)((reg >> i) & 1));
    } else {
      Serial.print("#");
    }
  }
  Serial.println("");
}

void printDescriptors(DmacDescriptor *primaryDescriptor, DmacDescriptor *writebackDescriptor, 
  int16_t channelIndex) {
  DmacDescriptor *currentDescriptor = primaryDescriptor;
  uint32_t startAddr = (uint32_t)currentDescriptor;
  int16_t count = 0;
  if (!currentDescriptor->DESCADDR.bit.DESCADDR) {
    Serial.println("printDescriptors: ERROR - INVALID PRIMARY DESCRIPTOR");
  } else {
    while((uint32_t)currentDescriptor != startAddr || count == 0) {
      Serial.println("------------------------------------------------------------");
      Serial.println("{Descriptor for channel -> " + (String)channelIndex
        + "} {Number: "  + (String)count
        + "} {Address: "     + (uint32_t)currentDescriptor
        + "} {Link: "        + currentDescriptor->DESCADDR.bit.DESCADDR
        + "} {Source: "      + currentDescriptor->SRCADDR.bit.SRCADDR
        + "} {Destination: " + currentDescriptor->DSTADDR.bit.DSTADDR
        + "} {Count: "       + currentDescriptor->BTCNT.bit.BTCNT
        + "} {Valid: "       + currentDescriptor->BTCTRL.bit.VALID + "}"
      );
      count++;
      currentDescriptor = (DmacDescriptor*)currentDescriptor->DESCADDR.bit.DESCADDR;
      if (count > DMA_MAX_TASKS) {
        Serial.println("printDescriptors: ERROR - TIMEOUT");
        break;
      }
    }
  }
  Serial.println("------------------------------------------------------------");
  currentDescriptor = writebackDescriptor;
  Serial.println("{Writeback for channel -> " + (String)channelIndex
    + "} {Address: "     + (uint32_t)currentDescriptor
    + "} {Link: "        + currentDescriptor->DESCADDR.bit.DESCADDR
    + "} {Source: "      + currentDescriptor->SRCADDR.bit.SRCADDR
    + "} {Destination: " + currentDescriptor->DSTADDR.bit.DSTADDR
    + "} {Count: "       + currentDescriptor->BTCNT.bit.BTCNT
    + "} {Valid: "       + currentDescriptor->BTCTRL.bit.VALID + "}"
  );
  Serial.println("------------------------------------------------------------");
}


void printDesc(int16_t channel) {
  printDescriptors(&primaryDescriptorArray[channel], &writebackArray[channel], channel);
}






