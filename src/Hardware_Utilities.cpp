///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Pre-Processor & Forward Declarations
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "Hardware_Utilities.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Computer Communication
///////////////////////////////////////////////////////////////////////////////////////////////////

ComputerCom_ &ComputerCom;

bool ComputerCom_::manage(COMPUTERCOM_ACTION actionID, int32_t value) {
  switch(actionID) {
    case START: {
      COMPUTERCOM_BUS.begin(COMPUTERCOM_BAUDRATE);
      for (int16_t i = 0; i < timeout; i++) {
        if (COMPUTERCOM_BUS) {
          isActive = true;      
          return true;
        }
      }
      return false;
    }
    case SET_TIMEOUT: {
      if (value < COMPUTERCOM_TIMEOUT_MIN || value > COMPUTERCOM_TIMEOUT_MAX) {
        return false;
      } else {
        timeout = value;
        COMPUTERCOM_BUS.setTimeout(value);
        return true;
      }
    }
    case GET_TIMEOUT: {
      return COMPUTERCOM_BUS.getTimeout();
    }
    default : { 
      return false; 
    }
  }
} 
bool ComputerCom_::manage(COMPUTERCOM_ACTION actionID) { return manage(actionID, 0); }


int16_t ComputerCom_::write(Buffer<uint8_t> &sendBuffer, int16_t bytes) {
  if (!isActive) { init(); }
  int16_t availableBytes = COMPUTERCOM_BUS.availableForWrite();
  // Reduce num of bytes to write if not enough space in buffer.
  if (availableBytes < bytes) { 
    bytes = availableBytes; 
  }
  // Send the bytes to the computer.
  for (int16_t i = 0; i < bytes && i < COMPUTERCOM_BUFFER_SIZE; i++) {
    COMPUTERCOM_BUS.write(sendBuffer.remove());
  }
  return bytes;
}


bool ComputerCom_::read(Buffer<uint8_t> &recieveBuffer, int16_t bytes) {
  if (!isActive) { init(); }
  int16_t availableBytes = COMPUTERCOM_BUS.available();
  // Are there enough bytes available to read?
  if (availableBytes < bytes) {
    bytes = availableBytes;
  }
  // Read in bytes to the buffer
  for (int i = 0; i < bytes; i++) {
    recieveBuffer.add(COMPUTERCOM_BUS.read());
  }
  return bytes;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> I2C Bus
///////////////////////////////////////////////////////////////////////////////////////////////////

I2CBus::I2CBus(TwoWire &bus) {
  this->bus = bus;
}

bool I2CBus::manage(I2CBUS_ACTION actionID, int32_t value) {
  switch(actionID) {
    case START: {
      if (!isActive) {
        bus.begin();
        bus.setClock(clockSpeed);
        isActive = true;
      }
      return true;
    }
    case STOP: {
      if (isActive) {
        bus.end();
        isActive = false;
      }
      return true;
    }
    case RESET: {
      if (isActive) {
        bus.end();
        isActive = false;
      }
      bus.begin();
      // Should we reset settings?
      if (value == 0) {
        clockSpeed = I2C_DEFAULT_CLOCK_SPEED;
      }
      bus.setClock(clockSpeed);
      return true;
    }
    case SET_CLOCK_SPEED: {
      if (value < I2C_MIN_CLOCK_SPEED || value > I2C_MAX_CLOCK_SPEED) {
        return false;
      } else {
        clockSpeed = value;
        bus.setClock(clockSpeed);
        return true;
      }
    }
    case GET_CLOCK_SPEED: {
      return clockSpeed;
    }
    default: { // If ID is invalid...
      return false;
    }
  }
}

// TO DO -> Add bit modes -> 8/16/32 & MSB/LSB first/last/only
int16_t I2CBus::write(uint8_t *writeArray, int16_t bytes, uint8_t deviceAddress, 
  uint8_t registerAddress) {
  if (!isActive || writeArray == NULL || bytes < 0) {
    return -6;
  }
  // Ensure buffer size is not exceeded
  if (bytes > I2C_BUFFER_SIZE) { 
    return 0; 
  }
  bus.beginTransmission(deviceAddress);
  bus.write(registerAddress);
  // Write bytes to buffer.
  for (int16_t i = 0; i < bytes && i < I2C_BUFFER_SIZE; i++) {
    bus.write(writeArray[i]);
  }
  int16_t transmissionResult = bus.endTransmission() // Send bytes.
  if (transmissionResult == 0) {
    return bytes;
  } else {
    // TO DO -> ADD ERROR HANDLING HERE?
    return -transmissionResult;
  }
}


// TO DO -> Add bit modes -> 8/16/32 & MSB/LSB first/last/only
int16_t I2CBUS::read(uint8_t *readArray, int16_t bytes, uint8_t deviceAddress,
  uint8_t registerAddress) {
  if (!isActive || readArray == NULL || bytes < 0) {
    return -6;
  }
  // Request from device/register...
  bus.beginTransmission(deviceAddress);
  bus.write(registerAddress);
  int16_t transmissionResult = bus.endTransmission();
  // Check we got ACKN
  if (transmissionResult == 0) {
    bus.requestFrom(deviceAddress, bytes);
    int16_t availableBytes = bus.available();
    // Are there less bytes available then requested?
    if (availableBytes < bytes) {
      bytes = availableBytes;
    } // Read bytes into array.
    for (int16_t i = 0; i < bytes && i < I2C_BUFFER_SIZE; i++) {
      readArray[i] = bus.read();
    }
    return bytes;
  } else {
    return -transmissionResult;
  }
}

int16_t I2CBUS::scan(uint8_t *addressArray, uint16_t arraySize) {
  if (addressArray == NULL || arraySize < 0) {
    return -1;
  }
  int16_t devicesFound = 0;
  for (int16_t i = 0; i < 127; i++) {
    // Try to make contact with each address:
    bus.beginTransmission(i);
    int16_t transmissionResult = bus.endTransmission();
    if (transmissionResult == 0) { 
      // Success! Add address to array.
      addressArray[devicesFound] = i;
      devicesFound++;
      // Is array full?
      if (devicesFound == arraySize) {
        break;
      }
    }
  }
  return devicesFound;
}









