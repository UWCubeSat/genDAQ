
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

void DMAC_0_Handler(void) {
  
  //////// SET CURRENT CYCLES HERE ->

  DMAChannel &interruptSource = DMA[DMAC->INTPEND.bit.ID];

  // Get interrupt reason
  if (DMAC->INTPEND.bit.SUSP) { // Suspended
    DMAC->INTPEND.bit.SUSP = 1; // Clear flag
    
    // Determine error & update channel's flag

    // ON END OF SW TRANSFER -> CLEAR SW FLAG & SET CYCLES TO TRIGGER IF TRIGGER IS ACTIVE
    // ALSO ALWAYS RESET AT END OF TRANSFER

  }
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
  CH.CHCTRLA.reg |= settings.CHCTRLA_mask;          // Set channel control properties
  CH.CHPRILVL.bit.PRILVL = settings.priorityLevel;  // Set channel priority
  CH.CHINTENSET.reg |= DMAC_CHINTENSET_MASK;        // Enable all interrupts

  //Enable channel
  CH.CHCTRLA.bit.ENABLE = 1;

  // Set fields
  initialized = true;
  this->defaultCycles = settings.defaultCycles;
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


bool DMAChannel::setTasks(DMAChannelTask **tasks, int16_t numTasks) {  
  DmacDescriptor *currentDescriptor = nullptr; 
  DmacDescriptor *newDescriptor = nullptr;
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

  // Copy descriptor into primary storage
  memcpy(&primaryDescriptorArray[channelIndex], &tasks[0]->taskDescriptor, 
    sizeof(DmacDescriptor));

  // If active find index of currently executing task
  if (getStatus() == CHANNEL_STATUS_PAUSED) {
    DmacDescriptor *newDescriptor = &primaryDescriptorArray[channelIndex];
    currentDescriptor = &primaryDescriptorArray[channelIndex];

    // Iterate through linked list to find current task index
    for (int16_t i = 0; i < currentDescriptorCount; i++) {
      currentDescriptor = (DmacDescriptor*)currentDescriptor->DESCADDR.reg;
      if (writebackArray[channelIndex].DESCADDR.reg == (uint32_t)currentDescriptor) { // Index found 

        // Clear link if index is end/OOB new list or save index if not
        if (i >= numTasks - 1) {
          writebackArray[channelIndex].DESCADDR.reg &= ~DMAC_DESCADDR_MASK;
        } else {
          writebackIndex = i;
        }
        break;
      }
    }
  }
  // Link tasks together (if applicable) 
  if (numTasks > 1) {
    // Link primary task to next task -> begins linked list of tasks
    currentDescriptor = createDescriptor(&tasks[1]->taskDescriptor);
    primaryDescriptorArray[channelIndex].DESCADDR.reg = (uint32_t)currentDescriptor;

    // Link all other tasks in passed array (if applicable)
    for (int16_t i = 1; i < numTasks - 1 && i < DMA_MAX_TASKS - 1; i++) {
      DmacDescriptor *nextDescriptor = createDescriptor(&tasks[i + 1]->taskDescriptor); // Copy next descriptor
      currentDescriptor->DESCADDR.reg = (uint32_t)nextDescriptor;                       // Link current -> next
      currentDescriptor = nextDescriptor;                                               // Iterate "currentDescriptor"

      // If at index of executing descriptor link it
      if (i == writebackIndex + 1) {
        writebackArray[channelIndex].DESCADDR.reg = (uint32_t)nextDescriptor;
      }
    }
  }
  // Un-suspend channel (if applicable)
  if (!previouslySuspended) {
    start();
  }
  currentDescriptorCount = numTasks;
  return true;
}


bool DMAChannel::removeTask(int16_t taskIndex) {  
  DmacDescriptor *nextDescriptor = nullptr;   
  bool previouslySuspended = false;                 
  // Check for exceptions
  DMA_CHANNEL_STATUS currentStatus = getStatus();
  if (currentStatus == CHANNEL_STATUS_NULL
  || taskIndex < 0 
  || taskIndex >= currentDescriptorCount) {
    return false;
  }
  // If removing last descriptor disable channel, else suspend it
  if (currentDescriptorCount == 1) {
    // Stop channel if busy
    if (currentStatus == CHANNEL_STATUS_BUSY) {
      stop();
    } 
    // Clear executing descriptor's link & disable channel
    writebackArray[channelIndex].DESCADDR.reg &= ~DMAC_DESCADDR_MASK;
  } else {
    // Ensure channel is paused
    if (currentStatus == CHANNEL_STATUS_BUSY) {
      pause();
    } else if (currentStatus == CHANNEL_STATUS_PAUSED) {
      previouslySuspended = true;
    }
  }
  // Special case -> removing primary descriptor
  if (taskIndex == 0) {
    if (currentDescriptorCount > 1) { 
      // Shift 2nd descriptor down by copying it into primary slot
      nextDescriptor = (DmacDescriptor*)primaryDescriptorArray[channelIndex].DESCADDR.reg; // Save next descriptor to temp
      memcpy(&primaryDescriptorArray[channelIndex], &nextDescriptor,                       // Copy it into primary descriptor array
        sizeof(DmacDescriptor));                                            
      delete nextDescriptor;                                                               // Delete old copy (not in array)
    } else { 
      // Primary is only descriptor -> just reset it
      memset(&primaryDescriptorArray[channelIndex], 0, sizeof(DmacDescriptor));
    }
    currentDescriptorCount--;
    return true;
  }
  // Temp pointers -> for unlinking descriptor
  DmacDescriptor *currentDescriptor = &primaryDescriptorArray[channelIndex];
  DmacDescriptor *previousDescriptor = nullptr;

  // Iterate through descriptors until currentDescriptor = target
  for (int16_t i = 0; i < taskIndex; i++) {
    previousDescriptor = currentDescriptor;
    currentDescriptor = (DmacDescriptor*)currentDescriptor->DESCADDR.reg;
  }
  // Unlink target descriptor by changing links in adjacent descriptors
  if (currentDescriptor->DESCADDR.reg == ~DMAC_DESCADDR_MASK) { 
    previousDescriptor->DESCADDR.reg &= ~DMAC_DESCADDR_MASK; 
  } else {
    previousDescriptor->DESCADDR.reg &= (uint32_t)currentDescriptor->DESCADDR.reg;

    // Are we updating executing descriptor link? 
    if (getStatus() == CHANNEL_STATUS_PAUSED
    && writebackArray[channelIndex].DESCADDR.reg 
      == (uint32_t)currentDescriptor) {

      // Yes -> Link executing descriptor to new descriptor
      writebackArray[channelIndex].DESCADDR.reg      
        = (uint32_t)currentDescriptor->DESCADDR.reg;
    }
  }
  // Un-suspend channel (if applicable)
  if (!previouslySuspended) {
    start();
  }
  // Delete target
  delete currentDescriptor; 
  currentDescriptorCount--;
  return true;
}


bool DMAChannel::clearTasks() {                                                 
  // Check exceptions                                                                 
  if (currentDescriptorCount == 0) {
    return true;
  } else if (getStatus() == CHANNEL_STATUS_NULL) {
    return false;
  }
  // If channel is active stop it
  if (getStatus() != CHANNEL_STATUS_IDLE) stop();

  // Capture 2nd descriptor (if applicable)
  DmacDescriptor *nextDescriptor = nullptr;
  if (primaryDescriptorArray[channelIndex].DESCADDR.reg != DMAC_DESCADDR_MASK) {
    nextDescriptor = (DmacDescriptor*)primaryDescriptorArray[channelIndex].DESCADDR.reg;
  }
  // Cant delete primary descriptor because it is in SRAM so reset it.
  memset(&primaryDescriptorArray[channelIndex], 0, sizeof(DmacDescriptor));

  // Are there more descriptors?
  if (nextDescriptor != nullptr) {
    DmacDescriptor *currentDescriptor = nextDescriptor;

    // -> Iterate through remaining descriptors & delete them
    for (int16_t i = 1; i < currentDescriptorCount - 1; i++) {
      nextDescriptor = (DmacDescriptor*)currentDescriptor->DESCADDR.reg;
      delete currentDescriptor;
      currentDescriptor = nextDescriptor;
    }
    delete currentDescriptor; // Done this way to avoid casting pointer to address 0x000...
  }
  currentDescriptorCount = 0;
  return true;
}


bool DMAChannel::start(int16_t cycles) {
  DMA_CHANNEL_STATUS currentStatus = getStatus();
  if (currentDescriptorCount < 1) {
    return false;

  } else if (currentStatus == CHANNEL_STATUS_IDLE) {
    // If cycles OOB the resort to default
    if (cycles <= 0 || cycles > currentDescriptorCount) {
      cycles = defaultCycles;
    }
    remainingCycles = cycles;

    // Ensure channel is enabled & begin transfer
    if (!CH.CHCTRLA.bit.ENABLE == 0) {
      CH.CHCTRLA.bit.ENABLE = 1;                   // Enable channel
    }
    swTriggerFlag = true;                          // Set software trigger flag
    CH.CHCTRLB.bit.CMD = DMAC_CHCTRLB_CMD_NOACT;   // Disable software command (pause/resume)
    DMAC->SWTRIGCTRL.reg |= (1 << channelIndex);   // Trigger channel
    return true;

  } else if (currentStatus == CHANNEL_STATUS_PAUSED) {
    // If paused -> send resume command
    CH.CHCTRLB.bit.CMD |= DMAC_CHCTRLB_CMD_RESUME; // Send a "resume" command

  } else if (currentStatus == CHANNEL_STATUS_BUSY){ 
      // Increment remaining cycles counter
      remainingCycles += cycles;

  } else { // Chanel is not yet initialized...
    return false;
  }
}

bool DMAChannel::stop() {
  // Check exceptions
  if (getStatus() == CHANNEL_STATUS_NULL) {
    return false;
  }
  // Stop transfer & sync
  CH.CHCTRLA.bit.ENABLE = 0;                          
  while(CH.CHINTFLAG.reg & DMAC_CHINTFLAG_MASK != 0); 

  // Reset software command
  CH.CHCTRLB.bit.CMD = DMAC_CHCTRLB_CMD_NOACT;

  // Reset fields
  swTriggerFlag = false;                              
  remainingCycles = 0;
  paused = false;
  pauseFlag = false;
  activeFlag = false;

  return true;
}

bool DMAChannel::pause() {
  // Check exceptions
  DMA_CHANNEL_STATUS currentStatus = getStatus();
  if (currentStatus == CHANNEL_STATUS_NULL) {
    return false;
  } else if (DMA_CHANNEL_STATUS == CHANNEL_STATUS_PAUSED) {
    return true;
  }
  // Pause current transfer
  CH.CHINTENCLR.bit.SUSP = 1;                     // Disable suspend interrupts           
  while(CH.CHINTENCLR.bit);                       // Wait for sync
  CH.CHCTRLB.bit.CMD |= DMAC_CHCTRLB_CMD_SUSPEND; // Suspend transfer
  CH.CHINTENSET.bit.SUSP = 1;    

  paused = true; 
  return true;
}


/*
bool DMAChannel::enable() {
  // Check exceptions 
  if (currentTaskCount < 1) {
    return false;
  } else if (enabled) {
    disable();
  }
  // Check current status for exception
  DMA_CHANNEL_STATUS currentStatus = getStatus();
  if (currentStatus == CHANNEL_STATUS_NULL) {
    return false;
  }
  // Enable DMA channel & set flag
  CH.CHCTRLA.bit.ENABLE = 1; 
  enabled = true;

  // Clear software command
  CH.CHCTRLB.bit.CMD = 0;

  return true;
}


void DMAChannel::disable() {
  // Disable the channel
  CH.CHCTRLA.bit.ENABLE = 0;   
  while(CH.CHCTRLA.bit.ENABLE); // Wait for disable = transfer complete

  // Update class fields
  enabled = false;
  currentTaskCount = 0;
  suspendFlag = 0;
}


bool DMAChannel::trigger(DMA_TRIGGER_ACTION action, int16_t actionCount) { 
  // Check exceptions 
  DMA_CHANNEL_STATUS currentStatus = getStatus();
  if (currentStatus == CHANNEL_STATUS_NULL
  || currentStatus == CHANNEL_STATUS_DISABLED) {
    return false;
  }
  ///////////////////////////////// FIGURE OUT WHAT DO DO HERE

  // Update action count
  if (actionCount <= 0) {
    this->currentTaskCount = defaultActionCount;
  } else if (actionCount > 0) {
    this->currentTaskCount = actionCount;
  }
  // Update action
  uint8_t originalAction = CH.CHCTRLA.bit.TRIGACT;
  CH.CHCTRLA.bit.TRIGACT = (uint8_t)action;

  // Update class fields
  currentTaskCount = defaultActionCount;
  suspendFlag = false;

  // Trigger channel to begin transfer
  DMAC->SWTRIGCTRL.reg |= (1 << channelIndex);

  // Reset trigger action
  CH.CHCTRLA.bit.TRIGACT = originalAction;

  return true;
}


bool DMAChannel::suspend() {
  // Check exceptions
  DMA_CHANNEL_STATUS currentStatus = getStatus();
  if (currentStatus != CHANNEL_STATUS_IDLE
  && currentStatus != CHANNEL_STATUS_BUSY) {
    return false;
  }
  // Is there a pending suspend interrupt?
  if (CH.CHINTFLAG.bit.SUSP == 0) {
    // No -> suspend channel
    CH.CHINTENCLR.bit.SUSP = 1;  // Disable suspend interrupt                   
    CH.CHCTRLB.bit.CMD = 1;      // Suspend current transmission
    CH.CHINTENSET.bit.SUSP = 1;  // Re-enable suspend interrupt
  }
  suspendFlag = true;
  return true;
}

bool DMAChannel::resume() {
  // Ensure currently suspended
  if (getStatus() != CHANNEL_STATUS_SUSPENDED) {
    return false;
  }
  // Call resume cmd & clear flag
  CH.CHCTRLB.bit.CMD = 1;
  suspendFlag = false;
  channelError = CHANNEL_ERROR_NONE;

  return true;
}
*/





DMA_CHANNEL_STATUS DMAChannel::getStatus() {
  if (paused = true) {
    return CHANNEL_STATUS_PAUSED;
  } else if ((DMAC->BUSYCH.reg & (1 << channelIndex)) != 0      // Is transfer currently active?
           ||(DMAC->PENDCH.reg & (1 << channelIndex) != 0 )) {           // Is transfer currently  pending?
    return CHANNEL_STATUS_BUSY;
  } else if (initialized) {
    return CHANNEL_STATUS_IDLE;
  } else {
    return CHANNEL_STATUS_NULL;
  }
}


void DMAChannel::resetFields() {
  // State Variables //
  initialized = false;
  paused = false;

  // Interrupt Flags //
  pauseFlag = false;

  // Other //
  currentDescriptorCount = 0;
  remainingCycles = 0;
  defaultCycles = 0;
  externalTriggerCycles = 0;
  channelError = CHANNEL_ERROR_NONE;
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


.
inline void setDescriptor(DmacDescriptor &descriptor,
  uint8_t stepSize,
  uint8_t stepSelection,
  uint8_t destinationIncrement,
  uint8_t sourceIncrement,
  uint8_t beatSize,
  uint8_t blockAction,
  uint8_t eventOutputSelection,
  uint8_t descriptorValid,
  uint16_t blockTransferCount,
  uint32_t sourceAddress,
  uint32_t destinationAddress,
  uint32_t descriptorAddress) {

  descriptor.BT
  
}

// IF MEMCPY DOES NOT WORK...
  desc.BTCTRL.reg |= (DMAC_BTCTRL_MASK & tasks[0].taskDescriptor->BTCTRL.reg); // Copy control settings
  desc.BTCNT.bit.BTCNT = tasks[0].taskDescriptor->BTCNT.bit.BTCNT;             // Copy beat count for transfer
  desc.SRCADDR.bit.SRCADDR = tasks[0].taskDescriptor->SRCADDR.bit.SRCADDR;     // Copy source address
  desc.DSTADDR.bit.DSTADDR = tasks[0].taskDescriptor->DSTADDR.bit.DSTADDR;     // Copy destination address.
*/






