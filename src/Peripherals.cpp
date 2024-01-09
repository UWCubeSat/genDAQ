
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
void I2CdmaCallback(DMA_CALLBACK_REASON reason, DMAChannel &channel, 
  int16_t triggerType, int16_t descIndex, DMA_ERROR error) {

  // Get I2C IO instance
  I2CSerial *source = static_cast<I2CSerial*>(IOManager.getActiveIO(channel.getOwnerID()));

  // Do interrupt stuff here...
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

  // Initialize DMA & THEN settings! -> ORDER MATTERS
  initDMA();
  settings.setDefault();
  if (!init()) IOManager.abort(this);
}


bool I2CSerial::requestData(uint8_t deviceAddr, uint16_t registerAddr, bool reg16) {
  
  // If error, bus not idle or busy dma -> return false, else -> reset flags
  if (criticalError || s->I2CM.STATUS.bit.BUSSTATE != 1 || busyOpp != 0) return false;
  busyOpp = 1;
  dataRequested = true;
  
  // Save the device address & register length
  this->deviceAddr = deviceAddr;
  this->reg16 = reg16;
  int16_t regAddrLength = 1 + (int16_t)reg16; 

  if (reg16) { // Configure DMA and I2C peripheral to send 2 bytes
    this->registerAddr[0] = (uint8_t)(registerAddr >> 8);
    this->registerAddr[1] = (uint8_t)(registerAddr & UINT8_MAX);
    writeDesc->setTransferAmount(2);
    writeDesc->setIncrementConfig(true, true);
  
  } else { // Configure DMA and I2C peripheral to send 1 byte
    this->registerAddr[0] = registerAddr;
    writeDesc->setTransferAmount(1);
    writeDesc->setIncrementConfig(false, false);
  }

  // Set up DMA to write register addr
  writeDesc->setTransferAmount(regAddrLength);
  writeDesc->setSource((uint32_t)registerAddr, true);                                                 
  
  writeChannel->enableExternalTrigger();
  writeChannel->enable();
  readChannel->reset(true);
  
  // Set up the I2C module
                                                            
  s->I2CM.ADDR.reg = SERCOM_I2CM_ADDR_LENEN              // Enable auto length           
    | SERCOM_I2CM_ADDR_LEN(regAddrLength)                // Set length of data to write
    | SERCOM_I2CM_ADDR_ADDR(this->deviceAddr << 1 | 0);  // Load address of device into reg -> "0" denotes write req
  while(s->I2CM.SYNCBUSY.bit.SYSOP);                     // Sync

  return true;
}


bool I2CSerial::dataReady() {

  // If error or data not requested -> return false;
  if (criticalError || !dataRequested) return false;

  if (busyOpp == 0                         // DMA transfer was completed
  &&  s->I2CM.STATUS.bit.RXNACK == 0       // ACK was recieved on line
  &&  s->I2CM.STATUS.bit.BUSSTATE == 1) {  // Bus is idle 
      return true;                         // -> good to go!
  } else { 
    return false;                          // else -> not ready...
  } 
}


bool I2CSerial::readData(int16_t bytes, uint32_t dataDestination) {

  // If data not ready -> cannot read
  if (!dataReady()) return false;
  dataRequested = false;
  busyOpp = 2;

  // Set up DMA to read bytes
  readDesc->setTransferAmount(bytes);
  readDesc->setDestination(dataDestination, true);

  readChannel->enable();
  readChannel->reset(true);
  readChannel->enableExternalTrigger();

  // Send device addr on I2C line
  s->I2CM.ADDR.reg = SERCOM_I2CM_ADDR_LENEN        // Enable auto length           
    | SERCOM_I2CM_ADDR_LEN(bytes)                  // Set length of data to write
    | SERCOM_I2CM_ADDR_ADDR(deviceAddr << 1 | 1);  // Load address of device into reg -> "1" indicates read req
  while(s->I2CM.SYNCBUSY.bit.SYSOP);

  // Clear cached device addr 
  deviceAddr = 0;
  return true;
}


bool I2CSerial::writeData(uint8_t deviceAddr, uint16_t registerAddr, bool reg16, 
  int16_t writeCount, uint32_t dataSourceAddr) {

  // If critical error, busy bus, or busy opperation -> cannot write
  if (criticalError || s->I2CM.STATUS.bit.BUSSTATE != 1 || busyOpp != 0) {
    return false;
  }

  // Set up DMA to write bytes
  writeDesc->setTransferAmount(writeCount);
  writeDesc->setSource(dataSourceAddr, true);

  writeChannel->enable();
  writeChannel->reset(true);
  writeChannel->enableExternalTrigger();

  // Put address

}


// Write data


bool I2CSerial::init() {

  // Ensure given pins support I2CSerial transmission
  if (g_APinDescription[SCL].ulPinType != PIO_SERCOM 
  ||  g_APinDescription[SCL].ulPinType != PIO_SERCOM_ALT
  ||  g_APinDescription[SDA].ulPinType != PIO_SERCOM 
  ||  g_APinDescription[SDA].ulPinType != PIO_SERCOM_ALT) {
    IOManager.abort(this);
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

  // Disable & reset the I2CSerial module
  s->I2CM.CTRLA.bit.ENABLE = 0;
  while(s->I2CM.SYNCBUSY.bit.ENABLE);
  s->I2CM.CTRLA.bit.SWRST = 1;
  while(s->I2CM.SYNCBUSY.bit.SWRST);

  // Configure the I2CSerial module 
  s->I2CM.CTRLA.bit.MODE = 5;                // Enable master mode
  s->I2CM.CTRLB.bit.SMEN = 1;                // Enable "smart" mode -> Auto sends ACK on data read
  s->I2CM.BAUD.bit.BAUD                      // Calculate & set baudrate 
    = SERCOM_FREQ_REF / (2 * baudrate) - 7;  
  s->I2CM.INTENSET.bit.ERROR = 1;            // Enable error interrupt -> Other interrupt enabled by settings

  // Re-enable I2CSerial module
  s->I2CM.CTRLA.bit.ENABLE = 1;
  while(s->I2CM.SYNCBUSY.bit.ENABLE);

  // Set bus to idle & return
  s->I2CM.STATUS.bit.BUSSTATE = 1;
  while(s->I2CM.SYNCBUSY.bit.SYSOP);
  return true;
}


bool I2CSerial::initDMA() {

  // Allocate 2 new DMA channel
  DMAChannel *newChannel1 = DMA.allocateChannel(IOID);
  DMAChannel *newChannel2 = DMA.allocateChannel(IOID);
  if (newChannel1 == nullptr || newChannel2 == nullptr) return false;

  // Channels are not null -> set fields
  readChannel = newChannel1;
  writeChannel = newChannel2;

  readDesc = new TransferDescriptor();
  writeDesc = new TransferDescriptor();
  regDesc = new TransferDescriptor();

  // Configure descriptors/channels with base settings
  readDesc->
     setDataSize(I2C_READDESC_DATASIZE)
    .setAction(I2C_READDESC_TRANSFERACTION)
    .setIncrementConfig(I2C_READDESC_INCREMENTCONFIG_SOURCE, 
        I2C_READDESC_INCREMENTCONFIG_DEST)
    .setSource((uint32_t)&s->I2CM.DATA.reg, false);

  writeDesc-> 
     setDataSize(I2C_WRITEDESC_DATASIZE)
    .setAction(I2C_WRITEDESC_TRANSFERACTION)
    .setIncrementConfig(I2C_WRITEDESC_INCREMENTCONFIG_SOURCE,
        I2C_WRITEDESC_INCREMENTCONFIG_DEST)
    .setDestination((uint32_t)&s->I2CM.DATA.reg, false);                 //////// NOTE -> ALL DMAC SOURCE/DESTINATIONS MAY BE INCORRECT...

  

  readChannel->settings
    .setCallbackFunction(&I2CdmaCallback)
    .setBurstLength(I2C_READCHANNEL_BURSTLENGTH)
    .setTriggerAction(I2C_READCHANNEL_TRIGGERACTION)
    .setStandbyConfig(I2C_READCHANNEL_STANDBYCONFIG)
    .setExternalTrigger((DMA_TRIGGER)SERCOM_REF[sercomNum].DMAReadTrigger);

  writeChannel->settings
    .setCallbackFunction(&I2CdmaCallback)
    .setBurstLength(I2C_READCHANNEL_BURSTLENGTH)
    .setTriggerAction(I2C_READCHANNEL_TRIGGERACTION)
    .setStandbyConfig(I2C_READCHANNEL_STANDBYCONFIG)
    .setExternalTrigger((DMA_TRIGGER)SERCOM_REF[sercomNum].DMAWriteTrigger);

  // Add descriptors to channels (bind & loop them)
  readChannel->setDescriptor(readDesc, true, true);
  writeChannel->setDescriptor(writeDesc, true, true);

  // Disable external triggers by default
  readChannel->disableExternalTrigger();
  writeChannel->disableExternalTrigger();

  return true;
}


void I2CSerial::exit() {

  // Return ports to standard gpio
  PORT->Group[g_APinDescription[SCL].ulPort].PINCFG[g_APinDescription[SCL].ulPin].reg = 0;  
  PORT->Group[g_APinDescription[SDA].ulPort].PINCFG[g_APinDescription[SDA].ulPin].reg = 0;
  PORT->Group[g_APinDescription[SDA].ulPort].PMUX[g_APinDescription[SDA].ulPin >> 1].reg 
    |= ~PORT_PMUX_MASK;
  
  // Disable & reset sercom module/registers
  s->I2CM.CTRLA.bit.ENABLE = 0;
  while(s->I2CM.SYNCBUSY.bit.ENABLE);
  s->I2CM.CTRLA.bit.SWRST = 1;
  while(s->I2CM.SYNCBUSY.bit.SWRST);

  // Reset clock -> incase it was changed
  GCLK->PCHCTRL[SERCOM_REF[sercomNum].clock].bit.CHEN = 1; // This may not be necessary...

  // Clear pending interrupts & disable them
  NVIC_ClearPendingIRQ(SERCOM_REF[sercomNum].baseIRQ);
  NVIC_DisableIRQ(SERCOM_REF[sercomNum].baseIRQ);

  // DELETE OBJ BEFORE RESETTING!!!
  resetFields();
}


void I2CSerial::resetFields() {
  // Fields
  Sercom *s = nullptr;                   
  readChannel = nullptr;       
  writeChannel = nullptr;      
  readDesc = nullptr;  
  writeDesc = nullptr;
  bool reg16 = false;

  // Cache
  registerAddr[2] = { 0 };
  deviceAddr = 0;

  // State / Flags
  criticalError = false;
  busyOpp = 0;
  dataRequested = false;
}


I2CSerial::I2CSettings::I2CSettings(I2CSerial *super) {
  this->super = super;
}








