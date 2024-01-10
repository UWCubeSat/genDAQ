
#include <Peripherals.h>

IOManager_ &IOManager;


///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> IO Manager
///////////////////////////////////////////////////////////////////////////////////////////////////

IO *IOManager_::getActiveIO(int16_t IOID) { 
  ErrorSys.assert(activeIO[IOID] != nullptr, ASSERT_NULLPTR);    //////// POSSIBLY ADD SOME ERROR MITIGATION HERE...
  return activeIO[IOID]; 
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> PERIPHERAL BASE CLASS
///////////////////////////////////////////////////////////////////////////////////////////////////

int16_t IO::getBaseIOID() { return IOID; }

IO_TYPE IO::getBaseType() { return baseType; }

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> I2C BUS INTERRUPT FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

// Note -> interrupt handles errors only
void I2CdmaCallback(DMA_CALLBACK_REASON reason, TransferChannel &channel, 
  int16_t triggerType, int16_t descIndex, DMA_ERROR error) {

  I2CSerial *source = static_cast<I2CSerial*>(IOManager.getActiveIO(channel.getOwnerID()));
  source->busyOpp = 0;
  
}


///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> I2C BUS (SERIAL)
///////////////////////////////////////////////////////////////////////////////////////////////////

I2CSerial::I2CSerial(int16_t sercomNum, uint8_t SDA, uint8_t SCL, int16_t IOID) 
  : SDA(SDA), SCL(SCL) {

  resetFields();

  // Init derived type & ID in base class
  baseType = TYPE_I2CSERIAL;
  this->IOID = IOID;

  // Ensure sercom number is valid & get sercom ref
  this->sercomNum = sercomNum;
  if (sercomNum >= SERCOM_INST_NUM || sercomNum < 0) {
    ErrorSys.throwError(ERROR_I2C_OTHER);
    IOManager.abort(this);
  }
  s = GET_SERCOM(sercomNum); 

  // Initialize I2C peripheral
  if(!init()) IOManager.abort(this);
}


bool I2CSerial::requestData(uint8_t deviceAddr, uint16_t registerAddr, bool reg16) {
  
  // If error, bus not idle or busy dma -> return false, else -> reset flags
  if (criticalError || s->I2CM.STATUS.bit.BUSSTATE != I2C_BUS_IDLE_STATE 
  || busyOpp != 0) return false;
  busyOpp = 1;
  
  // Save the device address & register length
  this->deviceAddr = deviceAddr;
  updateReg(registerAddr, reg16);
  int16_t regAddrLength = 1 + (int16_t)reg16; 

  // Set up DMA to write register addr                                             
  writeChannel->enable();
  readChannel->resetTransfer(true);
  writeChannel->enableExternalTrigger();
  writeChannel->setDescriptor(regDesc, true);
  
  // Set up the I2C peripheral
                                                            
  s->I2CM.ADDR.reg = SERCOM_I2CM_ADDR_LENEN              // Enable auto length           
    | SERCOM_I2CM_ADDR_LEN(regAddrLength)                // Set length of data to write
    | SERCOM_I2CM_ADDR_ADDR(this->deviceAddr << 1        // Load address of device into reg -> "0" denotes write req
    | I2C_WRITE_TAG);  
  while(s->I2CM.SYNCBUSY.bit.SYSOP                       // Sync
    && s->I2CM.SYNCBUSY.bit.LENGTH);                     

  return true;
}


bool I2CSerial::dataReady() {

  // If error or data not requested -> return false;
  if (criticalError || deviceAddr == 0) return false;

  if (busyOpp == 0                         // DMA transfer was completed
  &&  s->I2CM.STATUS.bit.RXNACK == 0       // ACK was recieved on line
  &&  s->I2CM.STATUS.bit.BUSSTATE == 1) {  // Bus is idle 
      return true;                         // -> good to go!
  } else { 
    return false;                          // else -> not ready...
  } 
}


bool I2CSerial::readData(int16_t readCount, void *dataDestination) {

  // Handle exceptions
  MIN(readCount, 0);
  if (!dataReady() || readCount > I2C_MAX_READ) return false;
  busyOpp = 2;

  // Set up DMA to read bytes
  uint32_t destAddr = reinterpret_cast<uint32_t>(dataDestination);
  readDesc->setTransferAmount(readCount);
  readDesc->setDestination(destAddr, true);

  readChannel->enable();
  readChannel->resetTransfer(true);
  readChannel->setAllValid(true);
  readChannel->enableExternalTrigger();

  // Send device addr on I2C line
  s->I2CM.ADDR.reg = SERCOM_I2CM_ADDR_LENEN        // Enable auto length           
    | SERCOM_I2CM_ADDR_LEN(readCount)              // Set length of data to write
    | SERCOM_I2CM_ADDR_ADDR(deviceAddr << 1        // Load address of device into reg -> "1" indicates read req
    | I2C_READ_TAG);  
  while(s->I2CM.SYNCBUSY.bit.SYSOP                 // Sync
    && s->I2CM.SYNCBUSY.bit.LENGTH);

  // Clear cached device addr (also usd as req flag)
  deviceAddr = 0;
  return true;
}


bool I2CSerial::writeData(uint8_t deviceAddr, uint16_t registerAddr, bool reg16, 
  int16_t writeCount, void *dataSourceAddr) {
  // Handle exceptions
  MIN(writeCount, 0);
  if (criticalError || s->I2CM.STATUS.bit.BUSSTATE != 1 || busyOpp != I2C_BUS_IDLE_STATE
  || writeCount > I2C_MAX_WRITE) {
    return false;
  }
  this->deviceAddr = 0;
  busyOpp = 3;

  // Update register cache & descriptor
  updateReg(registerAddr, reg16);
  uint8_t regAddrLength = 1 + (uint8_t)reg16;

  // Set up DMA to write bytes
  uint32_t sourceAddr = reinterpret_cast<uint32_t>(dataSourceAddr);
  writeDesc->setTransferAmount(writeCount);
  writeDesc->setSource(dataSourceAddr, true);

  TransferDescriptor *descs[2] = { regDesc, writeDesc };

  writeChannel->enable();
  writeChannel->resetTransfer(true);
  writeChannel->setDescriptors(descs, 2, true, false);
  writeChannel->enableExternalTrigger();

  // Enable I2C peripheral
  s->I2CM.ADDR.reg = SERCOM_I2CM_ADDR_LENEN        // Enable auto length           
    | SERCOM_I2CM_ADDR_LEN(regAddrLength)          // Set length of data to write
    | SERCOM_I2CM_ADDR_ADDR(deviceAddr << 1        // Load address of device into reg -> "0" denotes write req
    | I2C_WRITE_TAG); 
  while(s->I2CM.SYNCBUSY.bit.SYSOP                 // Sync
    && s->I2CM.SYNCBUSY.bit.LENGTH);

  return true;
}


bool I2CSerial::dataReady() { 
  return (deviceAddr == 0
       && busyOpp == 0 
       && !criticalError
       && s->I2CM.STATUS.bit.BUSSTATE == 1 
       && s->I2CM.STATUS.bit.RXNACK == 0); 
}


bool I2CSerial::isBusy() { 
  return (busyOpp != 0
       || s->I2CM.STATUS.bit.BUSSTATE == 3
       || s->I2CM.STATUS.bit.BUSSTATE == 2); 
}


bool I2CSerial::resetBus(bool hardReset) {
  // If DMA active -> stop it
  if (busyOpp != 0) {
    writeChannel->disable(false);
    readChannel->disable(false);
    busyOpp = 0;
  }

  if (hardReset) {
    exit();
    if(!init()) {
      return false;
    }
  } else {
    // Send stop cmd & attempt to force idle state
    s->I2CM.CTRLB.bit.CMD = I2C_STOP_CMD;
    while(s->I2CM.SYNCBUSY.bit.SYSOP);
    s->I2CM.STATUS.bit.BUSSTATE = I2C_BUS_IDLE_STATE;
    while(s->I2CM.SYNCBUSY.bit.SYSOP);

    // If idle -> reset flags
    if (s->I2CM.STATUS.bit.BUSSTATE == I2C_BUS_IDLE_STATE) {
      deviceAddr = 0;
      registerAddr[0] = 0;
      registerAddr[1] = 0;
    }
  }

  // Ensure bus state is correct
  if (s->I2CM.STATUS.bit.BUSSTATE != 1) {
    return false;
  } else {
    return true;
  }
}



bool I2CSerial::init() {

  // Ensure given pins support I2CSerial transmission
  if (g_APinDescription[SCL].ulPinType != PIO_SERCOM 
  ||  g_APinDescription[SCL].ulPinType != PIO_SERCOM_ALT
  ||  g_APinDescription[SDA].ulPinType != PIO_SERCOM 
  ||  g_APinDescription[SDA].ulPinType != PIO_SERCOM_ALT) {
    return false;
  }
  // Configure the pins
  PORT->Group[g_APinDescription[SCL].ulPort].PINCFG[g_APinDescription[SCL].ulPin].reg 
    = PORT_PINCFG_DRVSTR | PORT_PINCFG_PULLEN | PORT_PINCFG_PMUXEN;  
  PORT->Group[g_APinDescription[SDA].ulPort].PINCFG[g_APinDescription[SDA].ulPin].reg 
    = PORT_PINCFG_DRVSTR | PORT_PINCFG_PULLEN | PORT_PINCFG_PMUXEN;
  PORT->Group[g_APinDescription[SDA].ulPort].PMUX[g_APinDescription[SDA].ulPin >> 1].reg 
    = PORT_PMUX_PMUXO(g_APinDescription[SDA].ulPinType) | PORT_PMUX_PMUXE(g_APinDescription[SDA].ulPinType); // THIS MAY BE WRONG

  // Enable the sercom channel's interrupt vectors
  NVIC_ClearPendingIRQ(SERCOM_REF[sercomNum].baseIRQ);	
	NVIC_SetPriority(SERCOM_REF[sercomNum].baseIRQ, I2C_IRQ_PRIORITY);   
	NVIC_EnableIRQ(SERCOM_REF[sercomNum].baseIRQ); 

  // Enable the sercom channel's generic clock
  GCLK->PCHCTRL[SERCOM_REF[sercomNum].clock].bit.CHEN = 1;  // Enable clock channel
  GCLK->PCHCTRL[SERCOM_REF[sercomNum].clock].bit.GEN = 1;   // Enable clock generator

  // Disable & reset the I2CSerial peripheral
  s->I2CM.CTRLA.bit.ENABLE = 0;
  while(s->I2CM.SYNCBUSY.bit.ENABLE);
  s->I2CM.CTRLA.bit.SWRST = 1;
  while(s->I2CM.SYNCBUSY.bit.SWRST);

  // Configure the I2CSerial peripheral 
  s->I2CM.CTRLA.bit.MODE = 5;                // Enable master mode
  s->I2CM.CTRLB.bit.SMEN = 1;                // Enable "smart" mode -> Auto sends ACK on data read
  s->I2CM.BAUD.bit.BAUD                      // Calculate & set baudrate 
    = SERCOM_FREQ_REF / (2 * baudrate) - 7;  
  s->I2CM.INTENSET.bit.ERROR = 1;            // Enable error interrupt -> Other interrupt enabled by settings

  // Re-enable I2CSerial peripheral
  s->I2CM.CTRLA.bit.ENABLE = 1;
  while(s->I2CM.SYNCBUSY.bit.ENABLE);

  // Set bus to idle
  s->I2CM.STATUS.bit.BUSSTATE = 1;
  while(s->I2CM.SYNCBUSY.bit.SYSOP);

  // Initialize DMA & THEN settings! -> ORDER MATTERS
  initDMA();
  settings.setDefault();

  return true;
}


bool I2CSerial::initDMA() {

  // Allocate 2 new DMA channel
  TransferChannel *newChannel1 = DMA.allocateTransferChannel(IOID);
  TransferChannel *newChannel2 = DMA.allocateTransferChannel(IOID);
  if (newChannel1 == nullptr || newChannel2 == nullptr) return false;

  // Channels are not null -> set fields
  readChannel = newChannel1;
  writeChannel = newChannel2;

  readDesc = new TransferDescriptor(nullptr, nullptr, 0);
  writeDesc = new TransferDescriptor(nullptr, nullptr, 0);
  regDesc = new TransferDescriptor(nullptr, nullptr, 0);

  // Configure descriptors/channels with base settings
  readDesc->
     setDataSize(1)
    .setAction(ACTION_NONE)
    .setIncrementConfig(false, true)
    .setSource((uint32_t)&s->I2CM.DATA.reg, false);

  writeDesc-> 
     setDataSize(1)
    .setAction(ACTION_NONE)
    .setIncrementConfig(true, false)
    .setDestination((uint32_t)&s->I2CM.DATA.reg, false);                 //////// NOTE -> ALL DMAC SOURCE/DESTINATIONS MAY BE INCORRECT...

  regDesc-> 
     setDataSize(1)
    .setAction(ACTION_NONE)
    .setSource((uint32_t)&this->registerAddr)
    .setDestination((uint32_t)&s->I2CM.DATA.reg);

  readChannel->settings
    .setCallbackFunction(&I2CdmaCallback)
    .setBurstLength(1)
    .setTriggerAction(ACTION_TRANSFER_ALL)
    .setStandbyConfig(false)
    .setExternalTrigger((DMA_TRIGGER)SERCOM_REF[sercomNum].DMAReadTrigger)
    .setDescriptorsLooped(false, false);

  writeChannel->settings
    .setCallbackFunction(&I2CdmaCallback)
    .setBurstLength(1)
    .setTriggerAction(ACTION_TRANSFER_ALL)
    .setStandbyConfig(false)
    .setExternalTrigger((DMA_TRIGGER)SERCOM_REF[sercomNum].DMAWriteTrigger)
    .setDescriptorsLooped(false, false);

  // Add descriptors to channels (bind & loop them)
  readChannel->setDescriptor(readDesc, true);
  writeChannel->setDescriptor(writeDesc, true);

  // Disable external triggers by default
  readChannel->disableExternalTrigger();
  writeChannel->disableExternalTrigger();

  return true;
}


void I2CSerial::exit() {

  //If bus active -> send stop cmd
  if (s->I2CM.STATUS.bit.BUSSTATE != I2C_BUS_IDLE_STATE) {
    s->I2CM.CTRLB.bit.CMD = I2C_STOP_CMD;
    while(s->I2CM.SYNCBUSY.bit.SYSOP);
  }

  // Free DMA channels
  DMA.freeChannel(readChannel);
  DMA.freeChannel(writeChannel);

  // Return ports to standard gpio
  PORT->Group[g_APinDescription[SCL].ulPort].PINCFG[g_APinDescription[SCL].ulPin].reg = 0;  
  PORT->Group[g_APinDescription[SDA].ulPort].PINCFG[g_APinDescription[SDA].ulPin].reg = 0;
  PORT->Group[g_APinDescription[SDA].ulPort].PMUX[g_APinDescription[SDA].ulPin >> 1].reg 
    |= ~PORT_PMUX_MASK;
  
  // Disable & reset sercom peripheral/registers
  s->I2CM.CTRLA.bit.ENABLE = 0;
  while(s->I2CM.SYNCBUSY.bit.ENABLE);
  s->I2CM.CTRLA.bit.SWRST = 1;
  while(s->I2CM.SYNCBUSY.bit.SWRST);

  // Reset clock -> incase it was changed
  GCLK->PCHCTRL[SERCOM_REF[sercomNum].clock].bit.CHEN = 1; // This may not be necessary...

  // Clear pending interrupts & disable them
  NVIC_ClearPendingIRQ(SERCOM_REF[sercomNum].baseIRQ);
  NVIC_DisableIRQ(SERCOM_REF[sercomNum].baseIRQ);

  // Delete descriptors & reset fields
  delete readDesc;
  delete writeDesc;
  delete regDesc;
  resetFields();         
}


void I2CSerial::resetFields() {
  // Fields
  Sercom *s = nullptr;                   
  readChannel = nullptr;       
  writeChannel = nullptr;      
  readDesc = nullptr;  
  writeDesc = nullptr;
  regDesc = nullptr;
  bool reg16 = false;
  currentError = ERROR_NONE;

  // Cache
  registerAddr[2] = { 0 };
  deviceAddr = 0;

  // State / Flags
  criticalError = false;
  busyOpp = 0;
}


void I2CSerial::updateReg(int16_t registerAddr, bool reg16) {
  if (reg16) { // Configure DMA and I2C peripheral to send 2 bytes
    this->registerAddr[0] = (uint8_t)(registerAddr >> 8);
    this->registerAddr[1] = (uint8_t)(registerAddr & UINT8_MAX);
    regDesc->setTransferAmount(2);
    regDesc->setIncrementConfig(true, false);
  
  } else { // Configure DMA and I2C peripheral to send 1 byte
    this->registerAddr[0] = registerAddr;
    regDesc->setTransferAmount(1);
    regDesc->setIncrementConfig(false, false);
  }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> I2C SETTINGS
///////////////////////////////////////////////////////////////////////////////////////////////////


I2CSerial::I2CSettings::I2CSettings(I2CSerial *super) {
  this->super = super;
}


I2CSerial::I2CSettings &I2CSerial::I2CSettings::setBaudrate(uint32_t baudrate) {
  if (baudrate < I2C_MIN_BAUDRATE || baudrate > I2C_MAX_BAUDRATE) {
    settingsError = ERROR_SETTINGS_OOB;
    return *this;
  }
  // Disable, change baud rate & re-enable I2C peripheral (if enabled)
  if (super->s->I2CM.CTRLA.bit.ENABLE) {
    super->s->I2CM.CTRLA.bit.ENABLE = 0;
    while(super->s->I2CM.SYNCBUSY.bit.ENABLE);
    super->s->I2CM.BAUD.bit.BAUD = SERCOM_FREQ_REF / (2 * baudrate) - 7;
    super->s->I2CM.CTRLA.bit.ENABLE = 1;
    while(super->s->I2CM.SYNCBUSY.bit.ENABLE);
  } else {
    super->s->I2CM.BAUD.bit.BAUD = SERCOM_FREQ_REF / (2 * baudrate) - 7;
  }
  return *this;
}


I2CSerial::I2CSettings &I2CSerial::I2CSettings::setCallback(IOCallback *callbackFunction) {
  super->callback = callbackFunction;
  return *this;
}


I2CSerial::I2CSettings &I2CSerial::I2CSettings::setCallbackConfig(bool errorCallback, 
  bool requestCompleteCallback, bool readCompleteCallback, bool writeCompleteCallback) {
  super->errorCallback = errorCallback;
  super->requestCompleteCallback = requestCompleteCallback;
  super->readCompleteCallback = readCompleteCallback;
  super->writeCompleteCallback = writeCompleteCallback;
  return *this;
}


I2CSerial::I2CSettings &I2CSerial::I2CSettings::setSCLTimeoutConfig(bool enabled) {
  if (super->s->I2CM.CTRLA.bit.ENABLE) {
    super->s->I2CM.CTRLA.bit.ENABLE = 0;
    while(super->s->I2CM.SYNCBUSY.bit.ENABLE);
    super->s->I2CM.CTRLA.bit.LOWTOUTEN = (uint8_t)enabled;
    super->s->I2CM.CTRLA.bit.ENABLE = 1;
    while(super->s->I2CM.SYNCBUSY.bit.ENABLE);
  } else {
    super->s->I2CM.CTRLA.bit.LOWTOUTEN = (uint8_t)enabled;
  }
  return *this;
}


I2CSerial::I2CSettings &I2CSerial::I2CSettings::setInactiveTimeout(int16_t timeoutConfig) {
  if (timeoutConfig < 0 || timeoutConfig > I2C_MAX_SCLTIMEOUT_CONFIG) {
    settingsError = ERROR_SETTINGS_INVALID;
    return *this;
  }
  if (super->s->I2CM.CTRLA.bit.ENABLE) {
    super->s->I2CM.CTRLA.bit.ENABLE = 0;
    while(super->s->I2CM.SYNCBUSY.bit.ENABLE);
    super->s->I2CM.CTRLA.bit.INACTOUT = timeoutConfig;
    super->s->I2CM.CTRLA.bit.ENABLE = 1;
    while(super->s->I2CM.SYNCBUSY.bit.ENABLE);
  } else {
    super->s->I2CM.CTRLA.bit.INACTOUT = timeoutConfig;
  }
  return *this;
}









