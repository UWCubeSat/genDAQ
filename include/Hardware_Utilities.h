///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Pre-Processor & Forward Declarations
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "Global_Utilities.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Board
///////////////////////////////////////////////////////////////////////////////////////////////////
// To do -> controlling board? - Getting ram usage, reset ect...
class Board {
  public: 

  private:
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Computer Communication (singleton)
///////////////////////////////////////////////////////////////////////////////////////////////////

Serial_ &COMPUTERCOM_BUS = Serial;
const int32_t COMPUTERCOM_BAUDRATE = 250000;
const int16_t COMPUTERCOM_BUFFER_SIZE = SERIAL_BUFFER_SIZE;
const int16_t COMPUTERCOM_TIMEOUT_DEFAULT = 1000;
const int16_t COMPUTERCOM_TIMEOUT_MAX = 10000;
const int16_t COMPUTERCOM_TIMEOUT_MIN = 100;

enum COMPUTERCOM_ACTION {
  START,
  SET_TIMEOUT,
  GET_TIMEOUT
};

class ComputerCom_ {
  private:
    //// Properties ////
    int16_t timeout = COMPUTERCOM_TIMEOUT_DEFAULT;
    bool isActive = false;

  public:
    // @brief: Allows client to change computer communication
    //         settings.
    // @param: enum -> Specifies the action type to take.
    //          int -> Specifies the action's associated value
    //                 if applicable, if not don't specify value.
    // @return: True if successful, false otherwise.
    bool manage(COMPUTERCOM_ACTION, int32_t);
    bool manage(COMPUTERCOM_ACTION);

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

// TO DO -> Add support for devices that have timeout
//          feature in wire library.
TwoWire &I2C_DEFAULT_BUS = Wire;
const int32_t I2C_BUFFER_SIZE = 255; // Is there a definition for this?
const int32_t I2C_DEFAULT_CLOCK_SPEED = 100000;
const int32_t I2C_MAX_CLOCK_SPEED = 3000000;
const int32_t I2C_MIN_CLOCK_SPEED = 91254;

enum I2CBUS_ACTION {
  START,
  STOP,
  RESET,
  SET_CLOCK_SPEED,
  GET_CLOCK_SPEED
};

class I2CBus {
  private:
    //// Properties ////
    TwoWire &bus = I2C_DEFAULT_BUS;
    bool isActive = false;
    int32_t clockSpeed = I2C_DEFAULT_CLOCK_SPEED;

  public:
    I2CBus(TwoWire&);

    // @brief: Allows client to change I2C busses' settings
    //          & start/stop/restart bus.
    // @param: enum -> Specifies the action type to take.
    //          int -> The action's associated number, if
    //                 it is applicable, if not leave blank.
    // @return: True if successful, false otherwise.
    bool manage(I2CBUS_ACTION, int32_t);
    bool manage(I2CBUS_ACTION);

    // @brief: Writes bytes to a specific register.
    int16_t write(uint8_t*, int16_t, uint8_t, uint8_t); // TO DO -> Add different bit modes -> MSB LSB

    int16_t read(uint8_t*, int16_t, uint8_t, uint8_t); // TO DO -> Add different bit modes -> MSB LSB

    int16_t *scan(uint8_t*, int16_t);

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