///////////////////////////////////////////////////////////////////////////////////////////////////
///// FILE -> ADC
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <ADC.h>

static Adc *instances[ADC_MODULE_COUNT] = ADC_INSTS;

static uint16_t ADC_DATA_BUFFER[ADC_MODULE_COUNT][ADC_DB_LENGTH] = {};
static uint16_t ADC_DB_SIZE = sizeof(ADC_DATA_BUFFER[0]);
static uint16_t ADC_DBVAL_SIZE = sizeof(ADC_DATA_BUFFER[0][0]);

struct ADCInfo {
  DMA_TRIGGER dataTrigger;
  DMA_TRIGGER ctrlTrigger;
  uint8_t clockID;
  IRQn_Type baseIRQ;
};

static ADCInfo ADC_REF[ADC_MODULE_COUNT] {
  {TRIGGER_ADC0_RESRDY, TRIGGER_ADC0_SEQ, ADC0_GCLK_ID, ADC0_0_IRQn},
  {TRIGGER_ADC1_RESRDY, TRIGGER_ADC1_SEQ, ADC1_GCLK_ID, ADC1_0_IRQn} 
};

static ADCModule *modules[] = { nullptr };

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> ADC INTERRUPT
///////////////////////////////////////////////////////////////////////////////////////////////////

void ADC0_0_Handler(void) {
  ADCModule *source = nullptr;

  for (int16_t i = 0; i < ADC_MODULE_COUNT; i++) {
    ADCModule *targ = modules[i];
    if (targ != nullptr) {
      
    }
  }
}

void dataDMACallback (DMA_CALLBACK_REASON reason, TransferChannel &source, 
int16_t descriptorIndex, int16_t currentTrigger, ERROR_ID error) {

}

void ctrlDMACallback (DMA_CALLBACK_REASON reason, TransferChannel &source, 
int16_t descriptorIndex, int16_t currentTrigger, ERROR_ID error) {

}

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> ADC MODULE CLASS (PUBLIC METHODS)
///////////////////////////////////////////////////////////////////////////////////////////////////

ADCModule::ADCModule(uint8_t moduleNumber) {
  CLAMP(moduleNumber, 0, ADC_INST_NUM - 1);
  adc = instances[moduleNumber];
  adcNum = moduleNumber;
  DB = ADC_DATA_BUFFER[moduleNumber];
  resetFields();
  settings.setDefault();
}

bool ADCModule::begin() {
  if (!currentState) return true;
  if (modules[adcNum] != nullptr) return false;

  // Reset ADC & configure module
  adc->CTRLA.bit.SWRST = 1;
  while(adc->SYNCBUSY.bit.SWRST || adc->CTRLA.bit.SWRST); 

  adc->DSEQCTRL.reg = ADC_DSEQCTRL_MASK | ADC_DSEQCTRL_AUTOSTART;  // Allow DMA to change active pin
  GCLK->PCHCTRL[ADC_REF[adcNum].clockID].reg                       // Enable ADC clock
    = GCLK_PCHCTRL_GEN_GCLK1_Val | (1 << GCLK_PCHCTRL_CHEN_Pos); 

  resetFields();
  settings.setDefault();

  if (!setDescDefault()) return false;
  if (!initDMA()) return false;

  // Enable interrupts
  for (int16_t i = 0; i < ADC_IRQ_COUNT; i++) {
    NVIC_ClearPendingIRQ((IRQn_Type)(ADC_REF[adcNum].baseIRQ + i));
    NVIC_SetPriority((IRQn_Type)(ADC_REF[adcNum].baseIRQ + i), priorityLvl);
    NVIC_DisableIRQ((IRQn_Type)(ADC_REF[adcNum].baseIRQ + i));
  }
  modules[adcNum] = this;
  currentState = 1;
  return true;  
}

void ADCModule::end(bool blocking) {
  if (currentState == 0) return;

  exitDMA(blocking); 

  // Disable ADC
  adc->CTRLA.bit.ENABLE = 0;
  while(adc->SYNCBUSY.bit.ENABLE);

  adc->CTRLA.bit.SWRST = 1;
  while(adc->CTRLA.bit.SWRST = 1);

  modules[adcNum] = nullptr;
  settings.setDefault();
  resetFields();
}

bool ADCModule::addPin(uint8_t pinNum) {
  if (pinNum >= BOARD_PIN_COUNT
  || getPinValid(pinNum)
  || pinCount == ADC_MAX_PINS) {
     return false;
  }

  for (int16_t i = 0; i < ADC_MAX_PINS; i++) {
    if (pins[i] == -1) {
      pins[i] == pinNum;
      pinCount++;
      break;
    }
  }
  return true;
}

bool ADCModule::removePin(uint8_t pinNum) {
  if (pinNum > BOARD_PIN_COUNT || currentState != 1) return false;

  bool pinFound = false;
  for (int16_t i = 0; i < ADC_MAX_PINS; i++) {
    if (pins[i] == pinNum) {
      pins[i] = -1;
      pinCount--;
      pinFound = true;
      break;
    }
  }
  if (!pinFound) return false;
  return true;
}

bool ADCModule::enable() {
  if (!currentState) return false;
  if (currentState == 2) return true;

  while(syncBusy()); 

  // Configure pins
  for (int16_t i = 0; i < pinCount; i++) {

    if (pins[i] < PINS_COUNT) {

      if (PinManager.attachPin(pins[i])) {

        int16_t pinOdd = g_APinDescription[pins[i]].ulPin % 2;
        int16_t pinI = pins[i];

        // Configure pin w multiplexer/port module
        PORT->Group[g_APinDescription[pinI].ulPort].PMUX[pinI].reg 
            |= pinOdd ? PORT_PMUX_PMUXO(BOARD_ADC_PERIPH) : PORT_PMUX_PMUXE(BOARD_ADC_PERIPH);
        PORT->Group[g_APinDescription[pinI].ulPort].PINCFG[pinI].reg
            |= PORT_PINCFG_DRVSTR | PORT_PINCFG_PMUXEN;
        
        // Configure ADC with pin & add "select pin" req to DMA/ADC ctrl buffer
        adc->INPUTCTRL.bit.MUXPOS = g_APinDescription[pinI].ulADCChannelNumber;
        ctrlInput[ctrlIndex] = ADC_INPUTCTRL_MUXPOS_AIN0;
        ctrlIndex++;
        activePins++;

      } else {
        pins[i] == -1;
      }
    } else {
      // Configure non-hardware pins
      adc->INPUTCTRL.bit.MUXPOS = (pins[i] - 100); 
      activePins++;
    }
    while(adc->SYNCBUSY.bit.INPUTCTRL); 
  }

  if (erChannel > 0) {
    if (erDAC->CTRLA.bit.ENABLE) {
      currentError = ERROR_ADC_EXREF;

    } else {
      erDAC->CTRLA.bit.ENABLE = 0;              // Disable DAC
      while(erDAC->SYNCBUSY.bit.ENABLE);        // Sync

      erDAC->CTRLA.bit.SWRST = 1;               // Reset DAC
      while(erDAC->SYNCBUSY.bit.SWRST);         // Sync

      erDAC->CTRLB.bit.REFSEL = erType;         // Set reference type
      erDAC->DACCTRL[erChannel].bit.ENABLE = 1; // Enable specified channel

      erDAC->CTRLA.bit.ENABLE = 1;              // Re-Enable DAC
      while(erDAC->SYNCBUSY.bit.ENABLE);        // Sync
    }
  }

  // Start the DMA Channel
  dataChannel->enableExternalTrigger();
  dataChannel->setAllValid(true);

  ctrlChannel->enableExternalTrigger();
  ctrlChannel->setAllValid(true);

  dataChannel->enable();
  ctrlChannel->enable();

  // Enable & trigger ADC!
  adc->CTRLA.bit.ENABLE = 1;
  while(adc->SYNCBUSY.bit.ENABLE);

  adc->SWTRIG.bit.START = 1;
  while(adc->SYNCBUSY.bit.SWTRIG);

  currentState = 2;
  return true;
}

void ADCModule::disable() {
  if (currentState == 0 || currentState == 1) return;

  // Disable ADC & DMA
  adc->CTRLA.bit.ENABLE = 0;
  while(adc->SYNCBUSY.bit.ENABLE);

  dataChannel->disable(false);
  ctrlChannel->disable(false);

  if (currentState == 2) {
    flushBuffer();
    memset(ctrlInput, 0, sizeof(ctrlInput));
  }

  // Clear pending interrupts
  for (int16_t i = 0; i < ADC_IRQ_COUNT; i++) {
    NVIC_ClearPendingIRQ((IRQn_Type)(ADC_REF[adcNum].baseIRQ + i)); 
  }
  adc->INTFLAG.reg = ADC_INTFLAG_RESETVALUE; // Clear interrupt flags
  adc->SWTRIG.bit.FLUSH = 1;                 // Flush ADC cache
  while(adc->SYNCBUSY.bit.SWTRIG);           // Sync

  currentState = 1;
};

bool ADCModule::getPinValid(uint8_t pinNum) {
  if (adcNum == 0) {
    return (g_APinDescription[pinNum].ulPinAttribute & PIN_ATTR_ANALOG == 1);
  } else if (adcNum == 1) {
    #ifdef BOARD_FEATHER_M4_EXPRESS_CAN__ // Varient file is incorrect for this board...
      return (pinNum == 16 || pinNum == 17); 
    #else 
      return (g_APinDescription[adcNum].ulPinAttribute & PIN_ATTR_ANALOG_ALT == 1);
    #endif
  }
  return false;
}

void ADCModule::flushBuffer() {
  if (!currentState) return;
  bool irqPend = false;

  // IRQ = interrupt, DMB = data memory barrier
  if (currentState == 2 && __get_PRIMASK()) {
    irqPend = true;
    __disable_irq();
    __DSB();
  }
  // Flush using index or querey values @ increments
  if (DBIndex > 0) {
    memset(DB, 0, MAX(DBIndex + ADC_DEFAULT_DB_OVERCLEAR, ADC_DB_LENGTH) * ADC_DBVAL_SIZE);
    DBIndex = 0;
  } else {
    int16_t maxIndex;
    for (int16_t i = 0; i < ADC_DB_LENGTH; i += ADC_DB_INCREMENT) {
      if (DB[i] != 0) {
        maxIndex = i;
        memset(DB + i * ADC_DB_INCREMENT, 0, ADC_DB_INCREMENT * ADC_DBVAL_SIZE);
      } else {
        break;
      }
    }
    DBIndex = 0;
  }
  // Exit critical
  if (irqPend) {
    __DMB();  
    __enable_irq();
  }
}

bool ADCModule::syncBusy() {
  return (dataChannel->syncBusy() || ctrlChannel->syncBusy());
}

ADCModule::~ADCModule() { end(false); }

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> ADC MODULE SETTINGS
///////////////////////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> ADC MODULE CLASS (PRIVATE METHODS)
///////////////////////////////////////////////////////////////////////////////////////////////////

bool ADCModule::initDMA() {
  // Allocate new channels
  dataChannel = DMA.allocateChannel();
  ctrlChannel = DMA.allocateChannel();
  if (dataChannel != nullptr && ctrlChannel != nullptr) return false;

  // Get channel numbers
  dataChNum = dataChannel->getChannelNum();
  ctrlChNum = ctrlChannel->getChannelNum();

  // Set channel settings
  dataChannel->settings
    .setExternalTrigger(ADC_REF[adcNum].dataTrigger)
    .setTriggerAction(ACTION_TRANSFER_BURST)
    .setCallbackFunction(&dataDMACallback)
    .setCallbackConfig(false, false, true)
    .setDescriptorsLooped(true, false)
    .setPriorityLevel(priorityLvl);

  ctrlChannel->settings
    .setExternalTrigger(ADC_REF[adcNum].ctrlTrigger)
    .setTriggerAction(ACTION_TRANSFER_BURST)
    .setCallbackFunction(&ctrlDMACallback)
    .setCallbackConfig(false, false, true)
    .setPriorityLevel(priorityLvl)
    .setDescriptorsLooped(true, false);

  // Set & validate descriptors
  dataChannel->setDescriptor(&dataDesc, true);
  ctrlChannel->setDescriptor(&dataDesc, true);
  dataChannel->setAllValid(true);
  ctrlChannel->setAllValid(true);

  return true;
}

void ADCModule::exitDMA(bool blocking) {
  dataChannel->disable(blocking);
  ctrlChannel->disable(blocking);

  DMA.freeChannel(dataChannel);
  DMA.freeChannel(ctrlChannel);
}

void ADCModule::resetFields() {                                                 /////////// TO COMPLETE
  DMA.freeChannel(dataChannel);
  DMA.freeChannel(ctrlChannel);
  dataChannel = nullptr;
  ctrlChannel = nullptr;
  setDescDefault();

  memset(pins, -1, sizeof(pins));
  memset(ctrlInput, 0, sizeof(ctrlInput));
  flushBuffer();

  currentState = 0;
  for (int16_t i = 0; i < sizeof(pins); i++) pins[i] = -1;
  pinCount = 0;
  currentError = ERROR_NONE;
}

bool ADCModule::setDescDefault() {
  dataDesc
    .setAction(ACTION_SUSPEND)
    .setDataSize(2)
    .setIncrementConfig(false, true)
    .setTransferAmount(dataTransferSize)
    .setDestination((uint32_t)&DB, true)
    .setSource((uint32_t)&adc->RESULT.reg, false);

  ctrlDesc
    .setAction(ACTION_SUSPEND)
    .setDataSize(4)
    .setIncrementConfig(true, false)
    .setTransferAmount(1)
    .setSource(&ctrlInput, true)
    .setDestination((uint32_t)&adc->DSEQDATA.reg, false);

  return (ctrlDesc.isValid() && dataDesc.isValid());
}   