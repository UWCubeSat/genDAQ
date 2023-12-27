///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Pre-Processor & Forward Declarations
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "Hardware.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Hardware Interface
///////////////////////////////////////////////////////////////////////////////////////////////////

HardwareInterface_ &Board;

#if WIRE_INTERFACES_COUNT > 0
  int16_t I2CBusCount = 1;
  I2CBus I2CArray[1] = { 
    I2CBus(Wire) 
  };
#elif WIRE_INTERFACES_COUNT > 1
  int16_t I2CBusCount = 2;
  I2CBus I2CArray[2] = { 
    I2CBus(Wire), 
    I2CBus(Wire1) 
  };
#elif WIRE_INTERFACES_COUNT > 2
  int16_t I2CBusCount = 3;
  I2CBus I2CArray[3] = { 
    I2CBus(Wire), 
    I2CBus(Wire1), 
    I2CBus(Wire2) 
  };
#elif WIRE_INTERFACES_COUNT > 3
  int16_t I2CBusCount = 4;
  I2CBus I2CArray[4] = { 
    I2CBus(Wire), 
    I2CBus(Wire1), 
    I2CBus(Wire2), 
    I2CBus(Wire3), 
  };
#endif

I2CBus *HardwareInterface_::I2C(int16_t busID) {
  if (busID > I2CBusCount) { return nullptr; }
  return &I2CArray[busID];
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Computer Communication
///////////////////////////////////////////////////////////////////////////////////////////////////

//// External ////
ComputerCom_ &ComputerCom;

#if defined(SERIAL_PORT_MONITOR)
  Serial_ &COMPUTERCOM_BUS_DEFAULT = SERIAL_PORT_MONITOR;
#else 
  Serial_ &COMPUTERCOM_BUS_DEFAULT = Serial;
#endif

//// Fields ////
Serial_ &ComputerCom_::bus = COMPUTERCOM_BUS_DEFAULT;
int16_t ComputerCom_::timeout = COMPUTERCOM_TIMEOUT_DEFAULT;
bool ComputerCom_::isActive = false;


int32_t ComputerCom_::manage(COMPUTERCOM_ACTION_ID actionID, int32_t value) {
  switch(actionID) {
    case COMPUTERCOM_START: {
      bus.begin(COMPUTERCOM_BAUDRATE);
      for (int16_t i = 0; i < timeout; i++) {
        if (bus) {
          isActive = true;      
          return 1;
        }
        delay(1);
      }
      return 0;
    }
    case COMPUTERCOM_SET_TIMEOUT: {
      if (value < COMPUTERCOM_TIMEOUT_MIN || value > COMPUTERCOM_TIMEOUT_MAX) {
        return 0;
      } else {
        timeout = value;
        bus.setTimeout(value);
        return 1;
      }
    }
    case COMPUTERCOM_GET_TIMEOUT: {
      return timeout;
    }
    default : { 
      return 0; 
    }
  }
} 


int16_t ComputerCom_::write(Buffer<uint8_t> &sendBuffer, int16_t bytes) {
  if (!isActive) { init(); }
  int16_t availableBytes = bus.availableForWrite();
  // Check for space in both buffers
  if (availableBytes < bytes) { 
    bytes = availableBytes;
  }
  if (sendBuffer.currentSize() < bytes) {
    bytes = sendBuffer.currentSize();
  }
  // Send the bytes to the computer.
  for (int16_t i = 0; i < bytes && i < COMPUTERCOM_BUFFER_SIZE; i++) {
    bus.write(*sendBuffer.remove());
  }
  return bytes;
}


bool ComputerCom_::read(Buffer<uint8_t> &recieveBuffer, int16_t bytes) {
  if (!isActive) { init(); }
  int16_t availableBytes = bus.available();
  // Are there enough bytes available to read?
  if (availableBytes < bytes) {
    bytes = availableBytes;
  }
  if (recieveBuffer.remainingCapacity() < bytes) {
    bytes = recieveBuffer.remainingCapacity();
  }
  // Read in bytes to the buffer
  for (int i = 0; i < bytes; i++) {
    recieveBuffer.add(bus.read());
  }
  return bytes;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> I2C Bus
///////////////////////////////////////////////////////////////////////////////////////////////////

TwoWire &I2C_DEFAULT_BUS = Wire;


I2CBus::I2CBus(TwoWire &bus) {
  this->bus = bus;
}


int32_t I2CBus::manage(I2CBUS_ACTION_ID actionID, int32_t value) {
  switch(actionID) {
    case I2C_START: {
      if (!isActive) {
        bus.begin();
        bus.setClock(clockSpeed);
        #if defined(WIRE_HAS_TIMEOUT)
          bus.setTimeout(timeout);
        #endif
        isActive = true;
      }
      return 1;
    }
    case I2C_STOP: {
      if (isActive) {
        bus.end();
        isActive = false;
      }
      return 1;
    }
    case I2C_RESET: {
      if (isActive) {
        bus.end();
      }
      bus.begin();
      isActive = true;
      // Should we reset settings?
      if (value == 0) {
        clockSpeed = I2C_DEFAULT_CLOCK_SPEED;
        timeout = I2C_DEFAULT_TIMEOUT;
      } // Else = do nothing...
      bus.setClock(clockSpeed);
      #if defined(WIRE_HAS_TIMEOUT)
        bus.setTimeout(timeout);
      #endif
      return 1;
    }
    case I2C_SET_CLOCK_SPEED: {
      if (value < I2C_MIN_CLOCK_SPEED || value > I2C_MAX_CLOCK_SPEED) {
        return 0;
      } else {
        clockSpeed = value;
        if (isActive) {
          bus.setClock(clockSpeed);
        }
        return 1;
      }
    }
    case I2C_GET_CLOCK_SPEED: {
      return clockSpeed;
    }
    case I2C_SET_TIMEOUT: {
      if (value < I2C_MIN_TIMEOUT || value > I2C_MAX_TIMEOUT) {
        return 0;
      } else {
        timeout = value;
        #if defined(WIRE_HAS_TIMEOUT)
          if (isActive) {
            bus.setTimeout(timeout);
          }
        #endif
        return 1;
      }
    }
    case I2C_GET_TIMEOUT: {
      return timeout;
    }
    default: { // If ID is invalid...
      return 0;
    }
  }
}


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
  int16_t transmissionResult = bus.endTransmission(); // Send bytes.
  if (transmissionResult == 0) {
    return bytes;
  } else {
    return -transmissionResult;
  }
}


int16_t I2CBus::read(uint8_t *readArray, int16_t bytes, uint8_t deviceAddress,
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


int16_t I2CBus::scan(uint8_t *addressArray, int16_t arraySize) {
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


///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> SPI Bus
///////////////////////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Analog Pin
///////////////////////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Digital Pin
///////////////////////////////////////////////////////////////////////////////////////////////////






