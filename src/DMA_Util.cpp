
//// PRE-PROCESSOR ////
#include <DMA_Util.h>
#define CH DMAC->Channel[channelIndex]

//// FORWARD DECLARATIONS ////
void DMAC_0_Handler(void);

void DMAC_1_Handler(void) __attribute__((weak, alias("DMAC_0_Handler")));  // Re-route all handlers to handler 0
void DMAC_2_Handler(void) __attribute__((weak, alias("DMAC_0_Handler")));
void DMAC_3_Handler(void) __attribute__((weak, alias("DMAC_0_Handler")));
void DMAC_4_Handler(void) __attribute__((weak, alias("DMAC_0_Handler")));

//// COMMON VARIABLES ////
__attribute__((__aligned__(16))) static DmacDescriptor 
  primaryDescriptorArray[DMA_MAX_CHANNELS] SECTION_DMAC_DESCRIPTOR,    
  writebackArray[DMA_MAX_CHANNELS] SECTION_DMAC_DESCRIPTOR;

///////////////////////////////////////////////////////////////////////////////////


DMATask::DMATask() {
  resetSettings();
}

DMATask &DMATask::setIncrementConfig(bool incrementDestination, bool incrementSource) {
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
    uint32_t targetAddress = 0;
    if (taskDescriptor.BTCTRL.bit.SRCINC) {
      if (taskDescriptor.BTCTRL.bit.STEPSEL) {
        targetAddress = startAddress + (taskDescriptor.BTCNT.bit.BTCNT 
          * (taskDescriptor.BTCTRL.bit.BEATSIZE + 1) 
          * (1 << taskDescriptor.BTCTRL.bit.STEPSIZE)); 
      } else {
        targetAddress = startAddress + (taskDescriptor.BTCNT.bit.BTCNT
          * (taskDescriptor.BTCTRL.bit.BEATSIZE + 1));
      }   
    } else {
      targetAddress = startAddress;
    }
    taskDescriptor.SRCADDR.bit.SRCADDR = targetAddress;
  }
  return *this;
}

DMATask &DMATask::setDestination(void *destinationPointer) {
  if (destinationPointer == nullptr) {
    // => ASSERT GOES HERE
  } else {
    uint32_t startAddress = (uint32_t)destinationPointer;
    uint32_t targAddress = 0;
    if (taskDescriptor.BTCTRL.bit.DSTINC) {
      if (taskDescriptor.BTCTRL.bit.STEPSEL) {
        targAddress = startAddress + (taskDescriptor.BTCNT.bit.BTCNT
          * (taskDescriptor.BTCTRL.bit.BEATSIZE + 1));
      } else {
        targAddress = startAddress + (taskDescriptor.BTCNT.bit.BTCNT
          * (taskDescriptor.BTCTRL.bit.BEATSIZE + 1)
          * (1 << taskDescriptor.BTCTRL.bit.STEPSIZE));
      }
    } else {
      targAddress = startAddress;
    }
    taskDescriptor.DSTADDR.bit.DSTADDR = targAddress;  
  }
  return *this;
}

void DMATask::resetSettings() {
  memset(&taskDescriptor, 0, sizeof(DmacDescriptor));
  taskDescriptor.BTCTRL.reg |= DMA_DEFAULT_DESCRIPTOR_MASK;
  taskDescriptor.BTCNT.bit.BTCNT = DMA_DEFAULT_ELEMENT_TRANSFER_COUNT;
}

///////////////////////////////////////////////////////////////////////////////////

DMAChannelSettings::DMAChannelSettings() {
  resetSettings();
}

DMAChannelSettings &DMAChannelSettings::setTransferThreshold(int16_t elements) {
  elements -= 1; 
  CLAMP(elements, 0, DMAC_CHCTRLA_THRESHOLD_8BEATS_Val);
  settingsMask &= ~DMAC_CHCTRLA_THRESHOLD_Msk;
  settingsMask |= DMAC_CHCTRLA_THRESHOLD(elements);
  return *this;
}

DMAChannelSettings &DMAChannelSettings::setBurstSize(int16_t elements) {
  elements -= 1;
  CLAMP(elements, 0, DMAC_CHCTRLA_BURSTLEN_16BEAT_Val);
  settingsMask &= ~DMAC_CHCTRLA_BURSTLEN_Msk;
  settingsMask |= DMAC_CHCTRLA_BURSTLEN((uint8_t)elements);
  return *this;
}

DMAChannelSettings &DMAChannelSettings::setTransferMode(DMA_TRANSFER_MODE transferMode) {
  settingsMask &= ~DMAC_CHCTRLA_TRIGACT_Msk;
  settingsMask |= DMAC_CHCTRLA_TRIGACT((uint8_t)transferMode);
  return *this;
}

DMAChannelSettings &DMAChannelSettings::setSleepConfig(bool enabledDurringSleep) {
  if (enabledDurringSleep) {
    settingsMask |= DMAC_CHCTRLA_RUNSTDBY;
  } else {
    settingsMask &= ~DMAC_CHCTRLA_RUNSTDBY;
  }
  return *this;
}

DMAChannelSettings &DMAChannelSettings::setPriorityLevel(DMA_PRIORITY_LEVEL level) {
  priorityLevel = (uint8_t)level;
  return *this;
}

DMAChannelSettings &DMAChannelSettings::setCallbackFunction(void (*callbackFunction)
  (DMA_INTERRUPT_REASON, DMAChannel&)) {
  this->callbackFunction = callbackFunction;
  return *this;
}

void DMAChannelSettings::removeCallbackFunction() {
  callbackFunction = nullptr;
}

void DMAChannelSettings::resetSettings() {
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

///////////////////////////////////////////////////////////////////////////////////

void DMAC_0_Handler(void) {
  // Get interrupt source & set up variables
  DMAChannel &interruptSource = DMA[DMAC->INTPEND.bit.ID];
  DMA_INTERRUPT_REASON callbackArg = SOURCE_UNKNOWN;

  // Has transfer completed? 
  if (DMAC->INTPEND.bit.TCMPL) {
    interruptSource.remainingActions--; 

    // Are there remaining actions?
    if (interruptSource.remainingActions > 1) {
      callbackArg = SOURCE_TASK_COMPLETE;

      // Is there a pending trigger? 
      if (DMAC->INTPEND.bit.PEND) {
        interruptSource.remainingActions += interruptSource.externalActions; // Yes -> Just increment
      } else {
        DMAC->SWTRIGCTRL.reg |= (1 << interruptSource.channelIndex); // No -> Send SW trigger
      }
    // Else -> No remaining actions
    } else {
      // Is there a trigger queued?
      if (DMAC->INTPEND.bit.PEND ) {
        callbackArg = SOURCE_TASK_COMPLETE;
      } else {
        callbackArg = SOURCE_CHANNEL_FREE;
      }
    }
  // Has channel been suspended?
  } else if (DMAC->INTPEND.bit.SUSP) {

    // Has channel been paused?
    if (DMAC->Channel[interruptSource.channelIndex].CHCTRLB.bit.CMD 
        = DMAC_CHCTRLB_CMD_SUSPEND) {
      callbackArg = SOURCE_CHANNEL_PAUSED;
    
    // Has there been a descriptor error?
    } else if (DMAC->INTPEND.bit.FERR){
      callbackArg = SOURCE_DESCRIPTOR_ERROR;                         // Set callback arg
      interruptSource.channelError = CHANNEL_ERROR_DESCRIPTOR_ERROR; // Set error
      interruptSource.stop();                                        // Disable channel
    } 
  // Has there been a transfer error?
  } else if (DMAC->INTPEND.bit.TERR) {

    // Yes -> Get/Set error type & disable channel
    if (DMAC->INTPEND.bit.CRCERR) {
      callbackArg = SOURCE_CRC_ERROR;
      interruptSource.channelError = CHANNEL_ERROR_CRC_ERROR;
    } else {
      callbackArg = SOURCE_TRANSFER_ERROR;
      interruptSource.channelError = CHANNEL_ERROR_TRANSFER_ERROR;
    }
    interruptSource.stop();
  }
  // If callback exists -> call it
  if (interruptSource.callbackFunction != nullptr) {
    interruptSource.callbackFunction(callbackArg, interruptSource);
  }
  // Clear all flags
  DMAC->INTPEND.bit.TCMPL = 1; 
  DMAC->INTPEND.bit.SUSP = 1;
  DMAC->INTPEND.bit.TERR = 1;
}


///////////////////////////////////////////////////////////////////////////////////

DMA_Utility &DMA;

bool DMA_Utility::begun = false;
int16_t DMA_Utility::channelCount = 0;


bool DMA_Utility::begin() { 
  end(); // Reset DMA  
  
  // Configure DMA settings
  DMAC->BASEADDR.bit.BASEADDR = (uint32_t)primaryDescriptorArray; // Set first descriptor SRAM address
  DMAC->WRBADDR.bit.WRBADDR = (uint32_t)writebackArray;           // Set writeback storage SRAM address
  DMAC->CTRL.reg |= DMAC_CTRL_LVLEN_Msk;                          // Enable all priority levels.
  MCLK->AHBMASK.bit.DMAC_ = 1;                                    // Enable DMA clock.

  // Enable round-robin schedualing for all priority lvls
  for (int16_t i = 1; i <= DMAC_LVL_NUM; i++) {
    DMAC->PRICTRL0.reg |= ((DMAC_PRICTRL0_RRLVLEN0_Pos + 1) * i) - 1;
  }

  // Enable interrupts and set to lowest possible priority
  for (int16_t i = 0; i < DMA_IRQ_COUNT; i++) {
    NVIC_SetPriority((IRQn_Type)(DMAC_0_IRQn + i), (1 << __NVIC_PRIO_BITS) - 1);  
    NVIC_EnableIRQ((IRQn_Type)(DMAC_0_IRQn + i));
  }

  // Re-enable DMAC
  DMAC->CTRL.bit.DMAENABLE = 1;
  begun = true; 
  return true;
}


bool DMA_Utility::end() {
  // Clear DMA registers
  DMAC->CTRL.bit.DMAENABLE = 0;                   // Disable system -> grants write access
  while(DMAC->CTRL.bit.DMAENABLE)                 // Sync
  DMAC->CTRL.bit.SWRST = 1;                       // Reset entire system                    
  MCLK->AHBMASK.bit.DMAC_ = 0;                    // Disable clock

    // Disable all interrupts
  for (int16_t i = 0; i < DMA_IRQ_COUNT; i++) {
    NVIC_ClearPendingIRQ((IRQn_Type)(DMAC_0_IRQn + i));  
    NVIC_DisableIRQ((IRQn_Type)(DMAC_0_IRQn + i));
  }

  // Reset proxy class' fields
  for (int16_t i = 0; i < DMA_MAX_CHANNELS; i++) {      
    channelArray[i].resetFields();
  }

  // Reset all transfer descriptors
  memset(primaryDescriptorArray, 0, sizeof(primaryDescriptorArray));
  memset(writebackArray, 0, sizeof(writebackArray));

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


///////////////////////////////////////////////////////////////////////////////////

DMAChannel::DMAChannel(const int16_t channelIndex) 
  : channelIndex(channelIndex) {}


bool DMAChannel::init(DMAChannelSettings &settings) {
  exit(); // Ensure channel cleared

  // Setup channel w/ settings
  CH.CHCTRLA.reg |= settings.settingsMask;          // Set channel control properties
  CH.CHPRILVL.bit.PRILVL = settings.priorityLevel;  // Set channel priority
  CH.CHINTENSET.reg |= DMAC_CHINTENSET_MASK;        // Enable all interrupts

  //Enable channel
  CH.CHCTRLA.bit.ENABLE = 1;

  // Set fields
  initialized = true;
  this->callbackFunction = settings.callbackFunction;    
  return true;
}


bool DMAChannel::exit() {
  // Reset channel registers
  CH.CHCTRLA.bit.ENABLE = 0;                     // Disable channel
  while(CH.CHCTRLA.bit.ENABLE);                  // Sync
  CH.CHCTRLA.bit.SWRST = 1;                      // Reset channel registers

  // Reset descriptors
  memset(&primaryDescriptorArray[channelIndex], 0, sizeof(DmacDescriptor));
  memset(&writebackArray[channelIndex], 0, sizeof(DmacDescriptor));

  resetFields();
  return true;
}


bool DMAChannel::updateSettings(DMAChannelSettings &settings) {
  // Ensure not busy & initialized
  if (getStatus() != CHANNEL_STATUS_IDLE
  && getStatus() != CHANNEL_STATUS_NULL) {
    return false;
  }
  //Disable channel -> grants write access
  CH.CHCTRLA.bit.ENABLE = 0;
  while(CH.CHCTRLA.bit.ENABLE);
  
  // Update settings
  CH.CHCTRLA.reg |= settings.settingsMask;
  CH.CHPRILVL.bit.PRILVL = settings.priorityLevel;

  return true;
}


bool DMAChannel::setTasks(DMATask **tasks, int16_t numTasks) {  
  // Establish temp variables
  DmacDescriptor *currentDescriptor = nullptr; 
  bool previouslySuspended = false;            
  int16_t writebackIndex = -1;

  // Check for exceptions
  DMA_CHANNEL_STATUS currentStatus = getStatus();
  if (currentStatus == CHANNEL_STATUS_NULL
  || (tasks == nullptr 
  || numTasks > DMA_MAX_TASKS 
  || numTasks <= 0)) { 
    return false;
  }
  // Check for null task array
  for (int i = 0; i < numTasks; i++) {
    if (tasks[i] == nullptr) return false;
  }
  // Special case -> currently busy
  if (currentStatus == CHANNEL_STATUS_BUSY) {
    pause();
  } else if (currentStatus == CHANNEL_STATUS_PAUSED) {
    previouslySuspended = true;
  }
  // Remove current tasks
  clearTasks();  

  // Copy first descriptor into primary storage
  memcpy(&primaryDescriptorArray[channelIndex], &tasks[0]->taskDescriptor, 
    sizeof(DmacDescriptor));

  // If active find index of currently executing task
  if (getStatus() == CHANNEL_STATUS_PAUSED) {
    currentDescriptor = &primaryDescriptorArray[channelIndex];

    // Iterate through linked list to find current task index
    for (int16_t i = 0; i < getTaskCount(); i++) {
      currentDescriptor = (DmacDescriptor*)currentDescriptor->DESCADDR.bit.DESCADDR;

      // Is the currently executing task at this index?
      if (writebackArray[channelIndex].DESCADDR.bit.DESCADDR 
          == (uint32_t)currentDescriptor) { 

        // Set link to start of new chain if not "in" chain
        if (i >= numTasks - 1) {
          writebackArray[channelIndex].DESCADDR.bit.DESCADDR 
            = (uint32_t)&primaryDescriptorArray[channelIndex];
        }
        break;
      }
    }
  }
  // Link tasks together (if applicable) 
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
    // Link last descriptor -> first (loop chain)
    currentDescriptor->DESCADDR.bit.DESCADDR 
      = (uint32_t)&primaryDescriptorArray[channelIndex];
  }
  // Un-suspend channel (if applicable)
  if (!previouslySuspended) {
    start();
  }
  return true;
}


bool DMAChannel::addTask(DMATask *task, int16_t taskIndex) {
  // Establish temp variables
  DMA_CHANNEL_STATUS currentStatus = getStatus();
  DmacDescriptor *previousDescriptor = &primaryDescriptorArray[channelIndex];
  DmacDescriptor *newDescriptor = nullptr;
  uint32_t previousDescriptorLink = 0;
  bool previouslySuspended = false;

  // Clamp taskIndex & check for status exception
  CLAMP(taskIndex, 0, getTaskCount() - 1);
  if (currentStatus == CHANNEL_STATUS_NULL
  || task == nullptr) {
    return false;
  }
  // Does channel need to be suspended?
  if (currentStatus == CHANNEL_STATUS_BUSY) {
    pause();
    previouslySuspended = false;
  }
  // Inserting at beggining?
  if (taskIndex == 0) {
    newDescriptor = createDescriptor(&primaryDescriptorArray[channelIndex]); // New descriptor = primary descriptor
    memcpy(&primaryDescriptorArray[channelIndex], &task->taskDescriptor,     // Copy added descriptor -> primary slot
      sizeof(DmacDescriptor));

    // Previous descriptor = last descriptor -> link = primary descriptor
    previousDescriptorLink = (uint32_t)&primaryDescriptorArray[channelIndex];
    
    // Link primary (added) -> new (old primary)
    primaryDescriptorArray[channelIndex].DESCADDR.bit.DESCADDR = (uint32_t)newDescriptor; 

  // Inserting at end? -> New descriptor = added descriptor
  } else if (taskIndex == getTaskCount() - 1) {
    newDescriptor = createDescriptor(&task->taskDescriptor);

    // Find last descriptor
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
  // Change link of active descriptor (if applicable)
  if (writebackArray[channelIndex].DESCADDR.bit.DESCADDR == previousDescriptorLink) {
    writebackArray[channelIndex].DESCADDR.bit.DESCADDR = (uint32_t)newDescriptor;
  }
  // Un-suspend channel (if applicable)
  if (!previouslySuspended) {
    start();
  }
  return true;
}


bool DMAChannel::removeTask(int16_t taskIndex) {  
  // Establish temp variables
  DMA_CHANNEL_STATUS currentStatus = getStatus();
  DmacDescriptor *currentDescriptor = nullptr;   
  int16_t removedAddress = 0;
  int16_t nextAddress = 0;
  bool previouslySuspended = false;

  // Clamp taskIndex & check for status exception
  CLAMP(taskIndex, 0, getTaskCount() - 1);
  if (currentStatus == CHANNEL_STATUS_NULL) {
    return false;
  }
  // Removing last descriptor? -> stop channel if active
  if (primaryDescriptorArray[channelIndex].DESCADDR.bit.DESCADDR == 0) {
    if (currentStatus == CHANNEL_STATUS_BUSY || CHANNEL_STATUS_PAUSED) {
      stop();
      writebackArray[channelIndex].DESCADDR.bit.DESCADDR = 0; 
    } 
    // Clear primary descriptor
    memset(&primaryDescriptorArray[channelIndex], 0, sizeof(DmacDescriptor));
    return true;
  // Else -> Ensure channel is paused
  } else {
    if (currentStatus == CHANNEL_STATUS_BUSY) {
      pause();
    } else if (currentStatus == CHANNEL_STATUS_PAUSED) {
      previouslySuspended = true;
    }
  }
  // Handle case -> removing primary descriptor
  if (taskIndex == 0) {
    // Shift 2nd descriptor down by copying it into primary slot
    currentDescriptor = (DmacDescriptor*)primaryDescriptorArray[channelIndex] 
      .DESCADDR.bit.DESCADDR;
    memcpy(&primaryDescriptorArray[channelIndex], &currentDescriptor,         
      sizeof(DmacDescriptor));    

    // Save addresses & delete target descriptor
    removedAddress = (uint32_t)currentDescriptor;
    nextAddress = currentDescriptor->DESCADDR.bit.DESCADDR;
    delete currentDescriptor;          

  // Handle case -> removing last task
  } else if (taskIndex == getTaskCount() - 1) {
    
    // Get 2nd to last task
    currentDescriptor = &primaryDescriptorArray[channelIndex];
    for (int16_t i = 0; i < taskIndex - 1; i++) {
      currentDescriptor = (DmacDescriptor*)currentDescriptor->DESCADDR.bit.DESCADDR;
    }
    // Save addresses & unlink/delete target descriptor
    removedAddress = currentDescriptor->DESCADDR.bit.DESCADDR;
    nextAddress = (uint32_t)&primaryDescriptorArray[channelIndex];
    delete (DmacDescriptor*)currentDescriptor->DESCADDR.bit.DESCADDR;
    currentDescriptor->DESCADDR.bit.DESCADDR = (uint32_t)&primaryDescriptorArray[channelIndex];
  
  // Handle case -> removing descriptor in middle of chain
  } else {
    DmacDescriptor *previousDescriptor = &primaryDescriptorArray[channelIndex];

    // Iterate through descriptors until currentDescriptor = target
    for (int16_t i = 0; i < taskIndex; i++) {
      previousDescriptor = currentDescriptor;
      currentDescriptor = (DmacDescriptor*)currentDescriptor->DESCADDR.bit.DESCADDR;
    }
    // Save addresses & unlink/delete target descriptor
    removedAddress = (uint32_t)currentDescriptor;
    nextAddress = currentDescriptor->DESCADDR.bit.DESCADDR;
    previousDescriptor->DESCADDR.bit.DESCADDR = currentDescriptor->DESCADDR.bit.DESCADDR;
    delete currentDescriptor;
  }
  // Update currently executing descriptor (if applicable)
  if (currentStatus == CHANNEL_STATUS_PAUSED
  && writebackArray[channelIndex].DESCADDR.bit.DESCADDR == removedAddress) {
    writebackArray[channelIndex].DESCADDR.bit.DESCADDR = nextAddress;
  }
  // Un-suspend channel (if applicable)
  if (!previouslySuspended) {
    start();
  }
  return true;
}


bool DMAChannel::clearTasks() {                                                 
  // Check exceptions                                                                 
  if (primaryDescriptorArray[channelIndex].SRCADDR.bit.SRCADDR == 0) {
    return true;
  } else if (getStatus() == CHANNEL_STATUS_NULL) {
    return false;
  }
  // If channel is active stop it
  if (getStatus() != CHANNEL_STATUS_IDLE) stop();

  // Capture 2nd descriptor (if applicable)
  DmacDescriptor *nextDescriptor = nullptr;
  if (primaryDescriptorArray[channelIndex].DESCADDR.bit.DESCADDR != 0) {
    nextDescriptor = (DmacDescriptor*)primaryDescriptorArray[channelIndex]
      .DESCADDR.bit.DESCADDR;
  }
  // Cant delete primary descriptor because it is in SRAM so reset it.
  memset(&primaryDescriptorArray[channelIndex], 0, sizeof(DmacDescriptor));

  // Are there more descriptors?
  if (nextDescriptor != nullptr) {
    DmacDescriptor *currentDescriptor = nextDescriptor;

    // -> Iterate through remaining descriptors & delete them
    while(currentDescriptor->SRCADDR.bit.SRCADDR != 0) {
      nextDescriptor = (DmacDescriptor*)currentDescriptor->DESCADDR.bit.DESCADDR;
      delete currentDescriptor;
      currentDescriptor = nextDescriptor;
    }
    delete currentDescriptor; // Done this way to avoid casting pointer to address 0x000...
  }
  return true;
}


int16_t DMAChannel::getTaskCount() {
  // Get primary descriptor. & create temp/count variables
  DmacDescriptor *currentDescriptor = &primaryDescriptorArray[channelIndex];
  int32_t firstDescriptor = (uint32_t)currentDescriptor;
  int16_t currentDescriptorCount = 0;
  
  // If primary descriptor is empty -> 0 tasks
  if (currentDescriptor->SRCADDR.bit.SRCADDR == 0) {
    return 0;

  // More then one descriptor -> iterate until linked = first descriptor
  } else {
    currentDescriptorCount = 1;
    while(currentDescriptor->DESCADDR.bit.DESCADDR != firstDescriptor) {
      currentDescriptor = (DmacDescriptor*)currentDescriptor->DESCADDR.bit.DESCADDR;
      currentDescriptorCount++;
    }
    return currentDescriptorCount;
  }
}


bool DMAChannel::start(int16_t actions) {
  // Handle tasks = 0
  if (actions == 0) {
    actions = getTaskCount();
  }
 // If no transfer descriptor -> cannot trigger
  DMA_CHANNEL_STATUS currentStatus = getStatus(); 
  if (primaryDescriptorArray[channelIndex].SRCADDR.bit.SRCADDR        
    == DMAC_SRCADDR_RESETVALUE) { 
    return false; 

  // If idle -> set counter/flag & trigger channel
  } else if (currentStatus == CHANNEL_STATUS_IDLE) { 
    CLAMP(actions, 1, DMA_MAX_CYCLES);
    remainingActions = actions;

    CH.CHCTRLB.bit.CMD = DMAC_CHCTRLB_CMD_NOACT; // Disable software command (pause/resume)
    DMAC->SWTRIGCTRL.reg |= (1 << channelIndex); // Trigger channel

  // If paused -> send resume command
  } else if (currentStatus == CHANNEL_STATUS_PAUSED) { 
    CH.CHCTRLB.bit.CMD |= DMAC_CHCTRLB_CMD_RESUME; 

  // If busy -> Increment remaining cycles counter
  } else if (currentStatus == CHANNEL_STATUS_BUSY){ 
    remainingActions += actions;

  // Else -> Invalid state...
  } else { 
    return false;
  }
  return true;
}

bool DMAChannel::stop() {
  // Check exceptions
  if (getStatus() == CHANNEL_STATUS_NULL) {
    return false;
  }
  // Stop, sync & restart channel
  CH.CHCTRLA.bit.ENABLE = 0;                          
  while(CH.CHCTRLA.bit.ENABLE != 0); 
  CH.CHCTRLA.bit.ENABLE = 1;

  // Reset command, counter & errors
  CH.CHCTRLB.bit.CMD = DMAC_CHCTRLB_CMD_NOACT;
  remainingActions = 0;
  channelError = CHANNEL_ERROR_NONE;
  return true;
}


bool DMAChannel::pause() {
  // Check exceptions
  DMA_CHANNEL_STATUS currentStatus = getStatus();
  if (currentStatus == CHANNEL_STATUS_NULL) {
    return false;
  } else if (currentStatus == CHANNEL_STATUS_PAUSED) {
    return true;
  }
  // Pause current transfer
  CH.CHINTENCLR.bit.SUSP = 1;                     // Disable suspend interrupts           
  while(CH.CHINTENCLR.bit.SUSP);                  // Wait for sync
  CH.CHCTRLB.bit.CMD |= DMAC_CHCTRLB_CMD_SUSPEND; // Suspend transfer
  CH.CHINTENSET.bit.SUSP = 1;    
  return true;
}


bool DMAChannel::enableExternalTrigger(DMA_TRIGGER_SOURCE source, int16_t actions) {
  // Handle tasks = 0
  if (actions == 0) {
    actions = getTaskCount();
  }

  // Check for exceptions
  DMA_CHANNEL_STATUS currentStatus = getStatus();
  if (currentStatus == CHANNEL_STATUS_NULL) { 
    return false;
  }
  // Clamp & set cycle count
  CLAMP(actions, 1, DMA_MAX_CYCLES);
  externalActions = actions;
  
  // Configure trigger
  CH.CHCTRLA.bit.ENABLE = 0;                // Disable channel
  while(CH.CHCTRLA.bit.ENABLE == 1);        // Sync
  CH.CHCTRLA.bit.TRIGSRC = (uint8_t)source; // Set trigger source
  CH.CHCTRLA.bit.ENABLE = 1;                // Re-enable channel

  return true;
}

bool DMAChannel::disableExternalTrigger() {
  // Check for exceptions
  DMA_CHANNEL_STATUS currentStatus = getStatus();
  if (CH.CHCTRLA.bit.TRIGACT == TRIGGER_NONE) {
    return true;
  } else if (currentStatus == CHANNEL_STATUS_NULL) {
    return false;
  }
  // Disable channel and reset trigger
  CH.CHCTRLA.bit.ENABLE = 0;                       
  while(CH.CHCTRLA.bit.ENABLE == 1);               
  CH.CHCTRLA.bit.TRIGSRC = (uint8_t)DMA_DEFAULT_TRIGGER_SOURCE;       
  CH.CHCTRLA.bit.TRIGACT = (uint8_t)DMA_DEFAULT_TRIGGER_ACTION;        
  CH.CHCTRLA.bit.ENABLE = 1; 

  return true;
}


bool DMAChannel::changeExternalTrigger(DMA_TRIGGER_SOURCE newSource, int16_t actions) {
  return enableExternalTrigger(newSource, actions);
}


DMA_CHANNEL_STATUS DMAChannel::getStatus() {
  if (CH.CHCTRLB.bit.CMD == DMAC_CHCTRLB_CMD_SUSPEND
  || channelError == CHANNEL_ERROR_DESCRIPTOR_ERROR) {
    return CHANNEL_STATUS_PAUSED;
  } else if (remainingActions > 0 ||(DMAC->PENDCH.reg & (1 << channelIndex) != 0 )) {  
    return CHANNEL_STATUS_BUSY;
  } else if (initialized) {
    return CHANNEL_STATUS_IDLE;
  } else {
    return CHANNEL_STATUS_NULL;
  }
}


DMA_CHANNEL_ERROR DMAChannel::getError() {
  return channelError;
}


void DMAChannel::resetFields() {
  channelError = CHANNEL_ERROR_NONE;
  initialized = false;
  remainingActions = 0;
  externalActions = 0;
  callbackFunction = nullptr;
}


DmacDescriptor *DMAChannel::createDescriptor(const DmacDescriptor *referenceDescriptor) {
  // Create descriptor in SRAM -> 128 bit aligned
  DmacDescriptor *newDescriptor = (DmacDescriptor*)aligned_alloc(16, sizeof(DmacDescriptor)); 

  // Copy values from reference into new descriptor.
  if (referenceDescriptor != nullptr) {                           
    memcpy(newDescriptor, referenceDescriptor, sizeof(DmacDescriptor)); 
  }                                                                                           
  return newDescriptor;  
}


///////////////////////////////////////////////////////////////////////////////////


/* If needed..
inline void clearDescriptor(DmacDescriptor &descriptor) {       
  descriptor.BTCTRL.reg   &= ~DMAC_BTCTRL_MASK;
  descriptor.BTCNT.reg    &= ~DMAC_BTCNT_MASK;
  descriptor.SRCADDR.reg  &= ~DMAC_SRCADDR_MASK;
  descriptor.DSTADDR.reg  &= ~DMAC_DSTADDR_MASK;
  descriptor.DESCADDR.reg &= ~DMAC_DESCADDR_MASK;
}

// IF MEMCPY DOES NOT WORK...
  desc.BTCTRL.reg |= (DMAC_BTCTRL_MASK & actions[0].taskDescriptor->BTCTRL.reg); // Copy control settings
  desc.BTCNT.bit.BTCNT = actions[0].taskDescriptor->BTCNT.bit.BTCNT;             // Copy beat count for transfer
  desc.SRCADDR.bit.SRCADDR = actions[0].taskDescriptor->SRCADDR.bit.SRCADDR;     // Copy source address
  desc.DSTADDR.bit.DSTADDR = actions[0].taskDescriptor->DSTADDR.bit.DSTADDR;     // Copy destination address.
*/






