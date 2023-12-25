///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Pre-Processor & Forward Declarations
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Global_Utilities.h>

class I2CBus;

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Hardware Interface
///////////////////////////////////////////////////////////////////////////////////////////////////

class HardwareInterface_ {
  private:
    static I2CBus I2CArray[];
    static int16_t I2CBusCount;

  public:
    I2CBus *I2C(int16_t);

  private:
    HardwareInterface_();
};
extern HardwareInterface_ &Board;

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Chip
///////////////////////////////////////////////////////////////////////////////////////////////////

enum CPU_PROPERTY_ID {
  BOARD_TYPE,
  BOARD_UNIQUE_ID,
  BOARD_FREE_RAM,
  BOARD_TOTAL_RAM,
  BOARD_CPU_TEMP
};

class CPU { // TO DO...
  public: 
    bool startWatchdog(int16_t);
    bool stopWatchdog();

    int32_t getProperty(CPU_PROPERTY_ID);

  private:
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Computer Communication (singleton)
///////////////////////////////////////////////////////////////////////////////////////////////////

extern Serial_ &COMPUTERCOM_BUS_DEFAULT;
const int32_t COMPUTERCOM_BAUDRATE = 250000;
const int16_t COMPUTERCOM_BUFFER_SIZE = SERIAL_BUFFER_SIZE;
const int16_t COMPUTERCOM_TIMEOUT_DEFAULT = 1000;
const int16_t COMPUTERCOM_TIMEOUT_MAX = 10000;
const int16_t COMPUTERCOM_TIMEOUT_MIN = 100;

enum COMPUTERCOM_ACTION_ID {
  COMPUTERCOM_START,
  COMPUTERCOM_SET_TIMEOUT,
  COMPUTERCOM_GET_TIMEOUT
};

class ComputerCom_ {
  private:
    //// Fields ////
    static Serial_ &bus;
    static int16_t timeout;
    static bool isActive;

  public:
    // @brief: Allows client to change computer communication
    //         settings.
    // @param: enum -> Specifies the action type to take.
    //          int -> Specifies the action's associated value
    //                 if applicable, if not don't specify value.
    // @return: Action's associated value or 1 if successful,
    //          & 0 otherwise.
    int32_t manage(COMPUTERCOM_ACTION_ID, int32_t = 0);

    // @brief: Sends bytes from the buffer to the computer.
    // @param: Buffer -> Contains the bytes to be sent.
    //            int -> Specifies how many bytes to send.
    // @return: The ~actual~ number of bytes sent.
    int16_t write(Buffer<uint8_t>&, int16_t);

    // @brief: Recieves bytes from the computer.
    // @param: Buffer -> Buffer which will store recieved bytes.
    //            int -> Max number of bytes to recieve.
    // @return: The ~actual~ number of bytes recieved.
    bool read(Buffer<uint8_t>&, int16_t);

  private:
    // #Constructor
    // @brief: Private constructor -> use "ComputerCom",
    //         extern/global instance to access class.
    ComputerCom_() {}; 
};
extern ComputerCom_ &ComputerCom;

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> I2C Bus
///////////////////////////////////////////////////////////////////////////////////////////////////

extern TwoWire &I2C_DEFAULT_BUS;
const int32_t I2C_BUFFER_SIZE = 255; // Is there a definition for this?
const int32_t I2C_DEFAULT_CLOCK_SPEED = 100000;
const int32_t I2C_MAX_CLOCK_SPEED = 3000000;
const int32_t I2C_MIN_CLOCK_SPEED = 91254;
const int16_t I2C_DEFAULT_TIMEOUT = 1000; // Is there a defininition for this?
const int16_t I2C_MIN_TIMEOUT = 100;
const int16_t I2C_MAX_TIMEOUT = 10000;

enum I2CBUS_ACTION_ID {
  I2C_START,
  I2C_STOP,
  I2C_RESET,
  I2C_SET_CLOCK_SPEED,
  I2C_GET_CLOCK_SPEED,
  I2C_SET_TIMEOUT,
  I2C_GET_TIMEOUT
};

class I2CBus {
  private:
    //// Fields ////
    TwoWire &bus = I2C_DEFAULT_BUS;
    bool isActive = false;
    int32_t clockSpeed = I2C_DEFAULT_CLOCK_SPEED;
    int16_t timeout;

  public:
    // #Constructor
    // @brief: Sets up I2CBus.
    // @param: TwoWire -> Wire object to use as bus.
    I2CBus(TwoWire&);

    // @brief: Allows client to change I2C busses' settings
    //          & start/stop/restart bus.
    // @param: enum -> Specifies the action type to take.
    //          int -> The action's associated number, if
    //                 it is applicable, if not leave blank.
    // @return: Action's associated value or 1 if successful,
    //          & 0 otherwise.
    int32_t manage(I2CBUS_ACTION_ID, int32_t = 0);

    // @brief: Writes bytes to a device's register.
    // @param: 1 -> Array of bytes to write to register.
    //         2 -> # of bytes to write (less/equal to array size).
    //         3 -> I2C bus address of device.
    //         4 -> Address of register to write to.
    // @return: Positive # -> Number of bytes that were written.
    //          Negative # -> Error code (no bytes written).
    int16_t write(uint8_t*, int16_t, uint8_t, uint8_t); // TO DO -> Add different bit modes -> MSB LSB

    // @brief: Reads in bytes from a device's register.
    // @param: 1 -> Array to store read bytes.
    //         2 -> # of bytes to read (less/equal to array size).
    //         3 -> I2C address of ~device~.
    //         4 -> Address of ~register~ to read from.
    // @return: Positive # -> Number of bytes that were read.
    //          Negative # -> Error code (no bytes read).
    int16_t read(uint8_t*, int16_t, uint8_t, uint8_t); // TO DO -> Add different bit modes -> MSB LSB

    // @brief: Scans I2C bus for device addresses.
    // @param: 1 -> Array of bytes to store device addresses.
    //         2 -> Size of array (max # of addresses).
    // @return: Number of addresses found/saved to array.
    int16_t scan(uint8_t*, int16_t);

  private:
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Serial Bus
///////////////////////////////////////////////////////////////////////////////////////////////////

class SerialBus {

};


///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> SPI Bus
///////////////////////////////////////////////////////////////////////////////////////////////////

class SPIBus {

};

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Digital Pin
///////////////////////////////////////////////////////////////////////////////////////////////////

class DigitalPin {

};

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Analog Pin
///////////////////////////////////////////////////////////////////////////////////////////////////

class AnalogPin {

};