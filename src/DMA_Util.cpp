
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
static DmacDescriptor __attribute__((__aligned__(16)))
  primaryDescriptorArray[DMA_MAX_CHANNELS] SECTION_DMAC_DESCRIPTOR,    
  writebackArray[DMA_MAX_CHANNELS] SECTION_DMAC_DESCRIPTOR;

bool DMA_Utility::begun = false;
int16_t DMA_Utility::channelCount = 0;
DMAChannel DMA_Utility::channelArray[] = {
  DMAChannel(0), DMAChannel(1), DMAChannel(2), DMAChannel(3), DMAChannel(4),
  DMAChannel(5), DMAChannel(6), DMAChannel(7), DMAChannel(8), DMAChannel(9)
};

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

void printDescriptors(DmacDescriptor *primaryDescriptor, int16_t channelIndex) {
  DmacDescriptor *currentDescriptor = primaryDescriptor;
  uint32_t startAddr = (uint32_t)primaryDescriptor;
  int16_t count = 0;
  Serial.println("------------------------------------------------------------");
  while((uint32_t)currentDescriptor != startAddr || count == 0) {
    Serial.println("{Descriptor for channel -> " + (String)channelIndex
      + "} {Number: "  + (String)count
      + "} {Address: "     + (uint32_t)currentDescriptor
      + "} {Link: "        + currentDescriptor->DESCADDR.bit.DESCADDR
      + "} {Destination: " + currentDescriptor->DSTADDR.bit.DSTADDR
      + "} {Source: "      + currentDescriptor->SRCADDR.bit.SRCADDR
      + "} {Valid: "       + currentDescriptor->BTCTRL.bit.VALID + "}"
    );
    Serial.println("------------------------------------------------------------");
    count++;
    currentDescriptor = (DmacDescriptor*)currentDescriptor->DESCADDR.bit.DESCADDR;
  }
  currentDescriptor = &writebackArray[channelIndex];
  Serial.println("{Writeback for channel -> " + (String)channelIndex
    + "} {Address: "     + (uint32_t)currentDescriptor
    + "} {Link: "        + currentDescriptor->DESCADDR.bit.DESCADDR
    + "} {Destination: " + currentDescriptor->DSTADDR.bit.DSTADDR
    + "} {Source: "      + currentDescriptor->SRCADDR.bit.SRCADDR
    + "} {Valid: "       + currentDescriptor->BTCTRL.bit.VALID + "}"
  );
  Serial.println("------------------------------------------------------------");
}

///////////////////////////////////////////////////////////////////////////////////////////////////


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
  (DMA_INTERRUPT_REASON, DMAChannel&)) {
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
    
    // Has there been a descriptor error?
    if (DMAC->INTPEND.bit.FERR){
      interruptSource.pauseFlag = true;                              // Set flag
      callbackArg = SOURCE_DESCRIPTOR_ERROR;                         // Set callback arg
      interruptSource.channelError = CHANNEL_ERROR_DESCRIPTOR_ERROR; // Set error
      interruptSource.stop();                                        // Disable channel
    
    } else {
      callbackArg = SOURCE_CHANNEL_PAUSED;
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


///////////////////////////////////////////////////////////////////////////////////////////////////

bool DMA_Utility::begin() { 
  end(); // Reset DMA  
  
  // Configure DMA settings
  DMAC->BASEADDR.reg = (uint32_t)primaryDescriptorArray;  // Set first descriptor SRAM address
  DMAC->WRBADDR.reg = (uint32_t)writebackArray;           // Set writeback storage SRAM address              
  MCLK->AHBMASK.bit.DMAC_ = 1;                            // Enable DMA clock.
  
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


///////////////////////////////////////////////////////////////////////////////////////////////////

DMAChannel::DMAChannel(const int16_t channelIndex) 
  : channelIndex(channelIndex) {}


bool DMAChannel::init(DMASettings &settings) {
  exit(); // Ensure channel cleared

  CH.CHCTRLA.reg |= settings.settingsMask;          // Set channel control properties
  CH.CHPRILVL.bit.PRILVL = settings.priorityLevel;  // Set channel priority
  CH.CHINTENSET.reg |= DMAC_CHINTENSET_MASK;        // Enable all interrupts

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


bool DMAChannel::updateSettings(DMASettings &settings) {
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
  DmacDescriptor *currentDescriptor = &primaryDescriptorArray[channelIndex];; 
  bool prevBusy = false;            
  int16_t writebackIndex = -1;
  int16_t nullTasks = 0;

  // Check for exceptions
  DMA_CHANNEL_STATUS currentStatus = getStatus();
  if (currentStatus == CHANNEL_STATUS_NULL
  || (tasks == nullptr 
  || numTasks > DMA_MAX_TASKS 
  || numTasks <= 0)) { 
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
    pause();
  }
  // Remove current tasks
  clearTasks();  

  // Copy first descriptor into primary storage
  memcpy(&primaryDescriptorArray[channelIndex], &tasks[0]->taskDescriptor, 
    sizeof(DmacDescriptor));

  // If active handle re-linking written-back descriptor/task
  if (getStatus() == CHANNEL_STATUS_PAUSED) {

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
  } else { // -> Only descriptor in list is primary one...
    currentDescriptor = &primaryDescriptorArray[channelIndex];
  }
  // Link last descriptor (currentDescriptor) -> first (loop chain)
  currentDescriptor->DESCADDR.bit.DESCADDR 
    = (uint32_t)&primaryDescriptorArray[channelIndex];

  // Un-suspend channel (if applicable)
  if (prevBusy) {
    start();
  }
  return true;
}


bool DMAChannel::addTask(DMATask *task, int16_t taskIndex) {
  // Establish temp variables
  DMA_CHANNEL_STATUS currentStatus = getStatus();
  DmacDescriptor *previousDescriptor = &primaryDescriptorArray[channelIndex];
  DmacDescriptor *newDescriptor = nullptr;
  bool prevBusy = false;

  // Clamp taskIndex & check for status exception
  CLAMP(taskIndex, 0, getTaskCount());
  if (currentStatus == CHANNEL_STATUS_NULL
  || task == nullptr) {
    return false;
  }
  // Does channel need to be suspended?
  if (currentStatus == CHANNEL_STATUS_BUSY) {
    prevBusy = true;
    pause();
  }
  // Inserting at beggining?
  if (taskIndex == 0) {
    newDescriptor = createDescriptor(&primaryDescriptorArray[channelIndex]); // New descriptor = primary descriptor
    memcpy(&primaryDescriptorArray[channelIndex], &task->taskDescriptor,     // Copy added descriptor -> primary slot
      sizeof(DmacDescriptor));
   
    // Link primary (added) -> new (old primary)
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
    start();
  }

 printDescriptors(&primaryDescriptorArray[channelIndex], channelIndex); ////////// TEMPORARY -> TO BE REMOVED //////////

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
  if (currentStatus == CHANNEL_STATUS_NULL) {
    return false;
  }
  // Removing last descriptor? -> clear tasks (disables channel)
  if (primaryDescriptorArray[channelIndex].DESCADDR.bit.DESCADDR == 0) {
    clearTasks();

  // Else -> Ensure channel is paused
  } else if (currentStatus == CHANNEL_STATUS_BUSY) {
      pause();
      prevBusy = true;
  }
  // Removing primary descriptor? -> Overwrite it with 2nd descriptor using memcpy
  if (taskIndex == 0) {
    currentDescriptor = (DmacDescriptor*)primaryDescriptorArray[channelIndex] 
      .DESCADDR.bit.DESCADDR;
    memcpy(&primaryDescriptorArray[channelIndex], &currentDescriptor,         
      sizeof(DmacDescriptor));    

    // Save addresses & delete copied (2nd) descriptor
    removedAddress = (uint32_t)currentDescriptor;
    nextAddress = currentDescriptor->DESCADDR.bit.DESCADDR;
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
    start();
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
  } else if (getStatus() == CHANNEL_STATUS_NULL) {
    return false;
  }
  // Disable channel
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
  // Can't delete primary descriptor so clear it...
  memset(&primaryDescriptorArray[channelIndex], 0, sizeof(DmacDescriptor));
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


bool DMAChannel::start(int16_t actions) {
  // Handle exceptions
  DMA_CHANNEL_STATUS currentStatus = getStatus();
  CLAMP(actions, 1, DMA_MAX_ACTIONS - remainingActions);
  if (primaryDescriptorArray[channelIndex].SRCADDR.bit.SRCADDR == 0) {
    return false;
  } else if (currentStatus == CHANNEL_STATUS_NULL) {
    return false;
  }
  // If channel is disabled re-enable it
  if (CH.CHCTRLA.bit.ENABLE == 0) {
    CH.CHCTRLA.bit.ENABLE = 1;
  }
  // If writeback descriptor is present & invalid, validate it
  if (writebackArray[channelIndex].BTCTRL.bit.VALID == 0
  && writebackArray[channelIndex].SRCADDR.bit.SRCADDR != 0) {
    writebackArray[channelIndex].BTCTRL.bit.VALID = 1;
  }
  // If idle -> increment actions & trigger channel 
  if (currentStatus == CHANNEL_STATUS_IDLE) { 
    remainingActions += actions;
    DMAC->SWTRIGCTRL.reg |= (1 << channelIndex); // Trigger channel

  // If paused -> clear flag & send resume command
  } else if (currentStatus == CHANNEL_STATUS_PAUSED) { 
    pauseFlag = false;
    CH.CHCTRLB.bit.CMD |= DMAC_CHCTRLB_CMD_RESUME; 

  // If busy -> Increment remaining cycles counter
  } else if (currentStatus == CHANNEL_STATUS_BUSY){ 
    remainingActions += actions;
  }
  return true;
}

bool DMAChannel::stop() {
  // Check exceptions
  DMA_CHANNEL_STATUS currentStatus = getStatus();
  if (currentStatus == CHANNEL_STATUS_NULL) {
    return false;
  }
  // If channel paused or busy -> disable, sync & re-enable channel
  if (currentStatus == CHANNEL_STATUS_PAUSED || CHANNEL_STATUS_BUSY) {
    CH.CHCTRLA.bit.ENABLE = 0;                          
    while(CH.CHCTRLA.bit.ENABLE != 0); 
    CH.CHCTRLA.bit.ENABLE = 1;
  }
  // Reset command, counter & errors
  CH.CHCTRLB.bit.CMD = DMAC_CHCTRLB_CMD_NOACT;
  remainingActions = 0;
  pauseFlag = false;
  channelError = CHANNEL_ERROR_NONE;
  return true;
}


bool DMAChannel::pause() {
  // Check exceptions
  DMA_CHANNEL_STATUS currentStatus = getStatus();
  if (currentStatus == CHANNEL_STATUS_NULL
  || primaryDescriptorArray[channelIndex].SRCADDR.bit.SRCADDR == 0) {
    return false;
  } else if (currentStatus == CHANNEL_STATUS_PAUSED) {
    return true;
  }
  // Set flag & pause channel
  pauseFlag = true;
  CH.CHCTRLB.bit.CMD |= DMAC_CHCTRLB_CMD_SUSPEND; 
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
  CLAMP(actions, 1, DMA_MAX_ACTIONS);
  externalActions = actions;
  
  // Configure trigger
  CH.CHCTRLA.bit.ENABLE = 0;                // Disable channel
  while(CH.CHCTRLA.bit.ENABLE);        // Sync
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
  if (pauseFlag) {
    return CHANNEL_STATUS_PAUSED;
  } else if (remainingActions > 0 ||((DMAC->PENDCH.reg & (1 << channelIndex)) != 0 )) {  
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
  pauseFlag = false;
  remainingActions = 0;
  externalActions = 0;
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


///////////////////////////////////////////////////////////////////////////////////////////////////


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






