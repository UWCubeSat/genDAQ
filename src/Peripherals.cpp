
#include <Peripherals.h>


I2CBus::I2CBus(Sercom *s, SERCOM *sercom, uint8_t SDA, uint8_t SCL) 
  : s(s), sercom(sercom), SDA(SDA), SCL(SCL) {

  deviceArray = nullptr;
  currentError = ERROR_NONE;
  deviceCount = 0;

  s->I2CM.CTRLB.bit.SMEN = 1;
  settings.setDefault();
  initI2C();
}



int16_t I2CBus::collectData(const uint32_t dataBufferAddr) {
  uint8_t dataCache[I2CBUS_CACHE_SIZE] = { 0 };
  int16_t cacheIndex = 0;

  if (sercom->isBusBusyWIRE()) {
    ErrorSys.throwError(ERROR_I2C_BUS_BUSY);
    return -1;
  }

  // Cycle through devices
  for (int16_t devNum = 0; devNum < deviceCount; devNum++) {
    I2CDevice &device = deviceArray[devNum];

    // Cycle through registers within a device
    for (int16_t regNum = 0; regNum < device.regCount; regNum++) {
      
      sercom->startTransmissionWIRE(device.addr, WIRE_WRITE_FLAG);
      sercom->sendDataMasterWIRE(device.regAddrArray[regNum]);
      sercom->prepareCommandBitsWire(WIRE_MASTER_ACT_STOP);
      sercom->startTransmissionWIRE(device.addr, WIRE_WRITE_FLAG);



      // Read all bytes from a given register -> using DMA...
      for (int16_t byteNum = 1; byteNum < device.regCountArray[regNum]; byteNum++) {

      }
    }
  }
}




void I2CBus::initI2C() {
  // Enable I2C peripheral
  sercom->initMasterWIRE(baudrate);
  sercom->enableWIRE();

  // Configure the SDA and SCL ports to be used for I2C
  pinPeripheral(SDA, g_APinDescription[SDA].ulPinType);
  pinPeripheral(SCL, g_APinDescription[SCL].ulPinType);
}




I2CBus::I2CSettings::I2CSettings(I2CBus *super) {
  this->super = super;
}








