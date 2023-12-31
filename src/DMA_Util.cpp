

#include <DMA_Util.h>

#define CH DMAC->Channel[channelNumber]


static __attribute__((__aligned__(16))) DmacDescriptor
  primaryDescriptorArray[DMA_MAX_CHANNELS] SECTION_DMAC_DESCRIPTOR,
  linkedDescriptorAray1[DMA_MAX_CHANNELS] SECTION_DMAC_DESCRIPTOR,
  linkedDescriptorArray2[DMA_MAX_CHANNELS] SECTION_DMAC_DESCRIPTOR,
  linkedDescriptorArray3[DMA_MAX_CHANNELS] SECTION_DMAC_DESCRIPTOR,
  writebackArray[DMA_MAX_CHANNELS] SECTION_DMAC_DESCRIPTOR;
  

DmacDescriptor *descriptorArrays[DMA_MAX_DESCRIPTORS] {
  primaryDescriptorArray,
  linkedDescriptorAray1,
  linkedDescriptorArray2,
  linkedDescriptorArray3
};

IRQn_Type DMAInterrupts[] = {
  DMAC_0_IRQn, 
  DMAC_1_IRQn, 
  DMAC_2_IRQn, 
  DMAC_3_IRQn, 
  DMAC_4_IRQn
};

///////////////////////////////////////////////////////////////////////////////////

void DMAC_0_Handler(void) {
  // Handler func called -> route to callback.
}


///////////////////////////////////////////////////////////////////////////////////

DMA_Utility &DMA;

bool DMA_Utility::begun = false;
int16_t DMA_Utility::channelCount = 0;



void DMA_Utility::begin() {
  begun = true;

  DMAC->CTRL.bit.DMAENABLE = 0;  // Disable system 
  DMAC->CTRL.bit.SWRST = 1;      // Reset System

  // Reset all channels
  for (int16_t i = 0; i < DMA_MAX_CHANNELS; i++) {  
    channelArray[i].end();
  }
  // Enable DMA clock
  MCLK->AHBMASK.bit.DMAC_ = 1;   
  
  // Set base descriptor addresses
  DMAC->BASEADDR.bit.BASEADDR = (uint32_t)primaryDescriptorArray; 
  DMAC->WRBADDR.bit.WRBADDR = (uint32_t)writebackArray;           

  // Reset all descriptors
  for (int16_t i = 0; i < DMA_MAX_DESCRIPTORS; i++) {    
    memset(descriptorArrays[i], 0, sizeof(DmacDescriptor));  
  }
  memset(writebackArray, 0, sizeof(writebackArray));  

  // Enable priority lvls.
  DMAC->CTRL.bit.LVLEN0 = 1;       
  DMAC->CTRL.bit.LVLEN1 = 1;  
  DMAC->CTRL.bit.LVLEN2 = 1;
  DMAC->CTRL.bit.LVLEN3 = 1;

  // Re-enable DMA system
  DMAC->CTRL.bit.DMAENABLE = 1;  

  // Enable interrupts and set to lowest possible priority
  for (int16_t i = 0; i < (sizeof DMAInterrupts / sizeof DMAInterrupts[0]); i++) {
    NVIC_EnableIRQ(DMAInterrupts[i]);  
    NVIC_SetPriority(DMAInterrupts[i], (1 << __NVIC_PRIO_BITS) - 1);  
  }
}



void DMA_Utility::end() {
  DMAC->CTRL.bit.DMAENABLE = 0; // Disable system 
  DMAC->CTRL.bit.SWRST = 1;     // Reset System

  // Disable all interrupts
  for (int16_t i = 0; i < (sizeof DMAInterrupts / sizeof DMAInterrupts[0]); i++) {
    NVIC_DisableIRQ(DMAInterrupts[i]);
  }
}



DMA_Channel &DMA_Utility::getChannel(uint8_t channelNumber) {
  if (channelNumber < 0) {
    channelNumber = 0;
  } else if (channelNumber >= DMA_MAX_CHANNELS) {  // Ensure requested channel is not OOB
    channelNumber = DMA_MAX_CHANNELS - 1;
  }
  return channelArray[channelNumber];
}


///////////////////////////////////////////////////////////////////////////////////

DMA_Channel::DMA_Channel(const uint8_t channelNumber) 
  : channelNumber(channelNumber) {}



bool DMA_Channel::begin(DMAChannelSettings *settings) {
  // Check for exceptions
  if (settings == nullptr || channelStatus == DMA_STATUS_SUSPENDED 
    || channelStatus == DMA_STATUS_BUSY) {  
    return false;
  }
  // Ensure channel cleared
  if (initialized) end();    

  // Setup channel w/ settings
  CH.CHCTRLA.bit.THRESHOLD = settings->transferThreshold; 
  CH.CHCTRLA.bit.BURSTLEN = settings->burstSize;
  CH.CHCTRLA.bit.TRIGACT = settings->triggerAction;
  CH.CHCTRLA.bit.TRIGSRC = settings->triggerSource;
  CH.CHCTRLA.bit.RUNSTDBY = settings->runStandby;
  CH.CHPRILVL.bit.PRILVL = settings->priorityLevel;

  // Enable all interrupts
  CH.CHINTENSET.reg |= DMAC_CHINTENSET_MASK;  
  
  // Enable channel & set flag
  CH.CHCTRLA.bit.ENABLE = 1;

  // Set fields
  this->settings = settings;    
  initialized = true; 
  return true;
}



bool DMA_Channel::end() {
  // Check for exceptions
  if (channelStatus != DMA_STATUS_IDLE) return false;

  // Reset fields
  initialized = false;
  settings = nullptr;

  // Dissable channel
  CH.CHCTRLA.bit.ENABLE = 0;  

  // Clear settings
  CH.CHCTRLA.bit.SWRST = 1;                      // Reset channel control reg
  CH.CHINTENCLR.reg &= ~DMAC_CHINTENCLR_MASK;    // Disable all interrupts
  CH.CHPRILVL.reg &= ~DMAC_CHPRILVL_MASK;        // Reset channel priority
  CH.CHINTFLAG.reg |= DMAC_CHINTFLAG_MASK;       // Clear channel flags
  DMAC->SWTRIGCTRL.reg &= ~(1 << channelNumber); // Clear software trigger

  // Reset transferdescriptors
  for (int16_t i = 0; i < DMA_MAX_DESCRIPTORS; i++) {
    memset(&descriptorArrays[i][channelNumber], 0, sizeof(DmacDescriptor));  // Dont know if this is right...
  }
  // Reset writeback descriptor
  memset(&writebackArray[channelNumber], 0, sizeof(writebackArray));

  return true;
}



bool DMA_Channel::addTask(DMAChannelTask *task) {
  // Check for exceptions
  if (task == nullptr || channelStatus != DMA_STATUS_IDLE) return false;
  
  // Add task to channel array
  this->tasks[task->taskNumber] = task;

  // Update task settings
  descriptorArrays[i]
  
}







