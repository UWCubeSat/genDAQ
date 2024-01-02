
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
  DMA_INTERRUPT_SOURCE callbackArg = SOURCE_TRANSFER_COMPLETE;
  // Get channel that called interrupt
  DMAChannel &interruptSource = DMA[DMAC->INTPEND.bit.ID];

  // Get reason for interrupt
  if (DMAC->INTPEND.bit.SUSP == 1) { // Channel suspended 
    DMAC->INTPEND.bit.SUSP = 1;      // -> clear flag

    // Is suspend due to descriptor error?
    if (DMAC->INTPEND.bit.FERR == 1) {   
      // Yes -> Set error status, stop channel & set cb arg                          
      interruptSource.channelError = CHANNEL_ERROR_DESCRIPTOR_ERROR;
      callbackArg = SOURCE_DESCRIPTOR_ERROR;
      interruptSource.stop();

    // Is suspend due to transfer completion?
    } else if (!interruptSource.paused) {
      interruptSource.remainingCycles--;

      // Is there pending trigger? -> If so it is external - increment counter...
      if (DMAC->INTPEND.bit.PEND == 1) {
        interruptSource.remainingCycles += interruptSource.externalTriggerCycles;
      } 
      // Reset channel
      DMAC->Channel[interruptSource.channelIndex].CHCTRLA.bit.ENABLE = 0;    // Dissable channel
      while(DMAC->Channel[interruptSource.channelIndex].CHCTRLA.bit.ENABLE); // Sync
      DMAC->Channel[interruptSource.channelIndex].CHCTRLA.bit.ENABLE = 1;    // Re-enable channel
      DMAC->Channel[interruptSource.channelIndex].CHCTRLB.bit.CMD = 0;       // Clear suspend cmd

      // Are there remaining cycles? -> If so trigger transfer & set cb arg
      if (interruptSource.remainingCycles > 0) {
        DMAC->SWTRIGCTRL.reg |= (1 << interruptSource.channelIndex); // Set trigger bit
        callbackArg = SOURCE_TRANSFER_COMPLETE;
      
      // Else -> Set cb arg to transfer complete
      } else {
        callbackArg = SOURCE_TRANSFER_COMPLETE;
      }
    // Else -> Suspend due to pause
    } else {
      callbackArg = SOURCE_CHANNEL_PAUSED;
    }
  } else if (DMAC->INTPEND.bit.) {

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
  // Establish temp variables
  DmacDescriptor *currentDescriptor = nullptr; 
  DmacDescriptor *newDescriptor = nullptr;
  bool previouslySuspended = false;            
  int16_t writebackIndex = -1;
  int16_t iterations = 0;

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
    while(currentDescriptor->DESCADDR.bit.DESCADDR != ~DMAC_DESCADDR_MASK) {
      currentDescriptor = (DmacDescriptor*)currentDescriptor->DESCADDR.reg;
      iterations++;
      if (writebackArray[channelIndex].DESCADDR.reg == (uint32_t)currentDescriptor) { // Index found 

        // Clear link if index is end/OOB new list or save index if not
        if (iterations >= numTasks - 1) {
          writebackArray[channelIndex].DESCADDR.reg &= ~DMAC_DESCADDR_MASK;
        } else {
          writebackIndex = iterations;
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
  return true;
}


bool DMAChannel::removeTask(int16_t taskIndex) {  
  DmacDescriptor *nextDescriptor = nullptr;   
  bool previouslySuspended = false;
  // Check for exceptions
  DMA_CHANNEL_STATUS currentStatus = getStatus();
  if (currentStatus == CHANNEL_STATUS_NULL
  || taskIndex < 0
  || taskIndex >= getTaskCount()) {
    return false;
  } 
  // If removing last descriptor disable channel, else suspend it
  if (primaryDescriptorArray[channelIndex].DESCADDR.bit.DESCADDR        
    == ~DMAC_DESCADDR_MASK) {
    // Stop channel if busy
    if (currentStatus == CHANNEL_STATUS_BUSY || CHANNEL_STATUS_PAUSED) {
      stop();
    } 
    // Clear executing descriptor's link
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
    if (primaryDescriptorArray[channelIndex].DESCADDR.bit.DESCADDR       
    != ~DMAC_DESCADDR_MASK) { 
      // Shift 2nd descriptor down by copying it into primary slot
      nextDescriptor = (DmacDescriptor*)primaryDescriptorArray[channelIndex].DESCADDR.reg; // Save next descriptor to temp
      memcpy(&primaryDescriptorArray[channelIndex], &nextDescriptor,                       // Copy it into primary descriptor array
        sizeof(DmacDescriptor));                                            
      delete nextDescriptor;   
                                                                  
    } else { 
      // Primary is only descriptor -> just reset it
      memset(&primaryDescriptorArray[channelIndex], 0, sizeof(DmacDescriptor));
    }
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
  return true;
}


bool DMAChannel::clearTasks() {                                                 
  // Check exceptions                                                                 
  if (primaryDescriptorArray[channelIndex].SRCADDR.bit.SRCADDR        
    == DMAC_SRCADDR_RESETVALUE) {
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
    while(currentDescriptor->SRCADDR.bit.SRCADDR != DMAC_SRCADDR_RESETVALUE) {
      nextDescriptor = (DmacDescriptor*)currentDescriptor->DESCADDR.reg;
      delete currentDescriptor;
      currentDescriptor = nextDescriptor;
    }
    delete currentDescriptor; // Done this way to avoid casting pointer to address 0x000...
  }
  return true;
}


int16_t DMAChannel::getTaskCount() {
  // Get primary descriptor & create temp count variable
  DmacDescriptor *currentDescriptor = &primaryDescriptorArray[channelIndex];
  int16_t currentDescriptorCount = 0;
  
  // If primary descriptor is empty -> 0 tasks
  if (currentDescriptor->SRCADDR.bit.SRCADDR == DMAC_SRCADDR_RESETVALUE) {
    return 0;

  } else {
    // More then one descriptor -> iterate until no link
    currentDescriptorCount = 1;
    while(currentDescriptor->DESCADDR.reg != ~DMAC_DESCADDR_MASK) {
      currentDescriptor = (DmacDescriptor*)currentDescriptor->DESCADDR.reg;
      currentDescriptorCount++;
    }
    return currentDescriptorCount;
  }
}


bool DMAChannel::start(int16_t cycles) {
  // Handle cases -> empty, idle, paused & busy
  DMA_CHANNEL_STATUS currentStatus = getStatus(); 
  if (primaryDescriptorArray[channelIndex].SRCADDR.bit.SRCADDR        
    == DMAC_SRCADDR_RESETVALUE) { 
    // If no transfer descriptor -> cannot trigger
    return false;

  } else if (currentStatus == CHANNEL_STATUS_IDLE) { 
    //Clamp & set cycles
    CLAMP(cycles, 1, DMA_MAX_CYCLES);
    remainingCycles = cycles;

    // Trigger transfer
    CH.CHCTRLB.bit.CMD = DMAC_CHCTRLB_CMD_NOACT;   // Disable software command (pause/resume)
    DMAC->SWTRIGCTRL.reg |= (1 << channelIndex);   // Trigger channel

  } else if (currentStatus == CHANNEL_STATUS_PAUSED) { 
    // If paused -> send resume command
    CH.CHCTRLB.bit.CMD |= DMAC_CHCTRLB_CMD_RESUME; 

  } else if (currentStatus == CHANNEL_STATUS_BUSY){ 
    // If busy -> Increment remaining cycles counter
    remainingCycles += cycles;

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

  // Reset software command
  CH.CHCTRLB.bit.CMD = DMAC_CHCTRLB_CMD_NOACT;

  // Reset flags/fields
  remainingCycles = 0;
  paused = false;

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

  paused = true; 
  return true;
}


bool DMAChannel::enableExternalTrigger(DMA_TRIGGER_SOURCE source, DMA_TRIGGER_ACTION action, 
  int16_t cycles) {
  // Check for exceptions
  DMA_CHANNEL_STATUS currentStatus = getStatus();
  if (currentStatus == CHANNEL_STATUS_NULL) { 
    return false;
  }
  // Clamp & set cycle count
  CLAMP(cycles, 1, DMA_MAX_CYCLES);
  externalTriggerCycles = cycles;
  
  // Configure trigger
  CH.CHCTRLA.bit.ENABLE = 0;                        // Disable channel
  while(CH.CHCTRLA.bit.ENABLE == 1);                // Sync
  CH.CHCTRLA.bit.TRIGSRC = (uint8_t)source;         // Set trigger source
  CH.CHCTRLA.bit.TRIGACT = (uint8_t)action;         // Set trigger action
  CH.CHCTRLA.bit.ENABLE = 1;                        // Re-enable channel

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


bool DMAChannel::changeExternalTrigger(DMA_TRIGGER_SOURCE newSource, DMA_TRIGGER_ACTION newAction,
  int16_t cycles) {
  return enableExternalTrigger(newSource, newAction, cycles);
}


DMA_CHANNEL_STATUS DMAChannel::getStatus() {
  if (paused = true) {
    return CHANNEL_STATUS_PAUSED;
  } else if ((DMAC->BUSYCH.reg & (1 << channelIndex)) != 0      // Is transfer currently active?
           ||(DMAC->PENDCH.reg & (1 << channelIndex) != 0 )) {  // Is transfer currently pending?
    return CHANNEL_STATUS_BUSY;
  } else if (initialized) {
    return CHANNEL_STATUS_IDLE;
  } else {
    return CHANNEL_STATUS_NULL;
  }
}

void DMAChannel::resetFields() {
  // Flags //
  initialized = false;
  paused = false;

  // Other //
  remainingCycles = 0;
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






