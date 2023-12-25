///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Pre-Processor & General Declarations
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Arduino.h"
#include <Global_Utilities.h>
#include <Codec.h>
#include <Hardware_Utilities.h>

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Request Manager
///////////////////////////////////////////////////////////////////////////////////////////////////

const int16_t REQ_LIST_INIT_SIZE = 16;
const int16_t REQ_LIST_MAX_SIZE = 32;

const int16_t ERROR_LIST_INIT_SIZE = 16;
const int16_t ERROR_LIST_MAX_SIZE = 32;

class RequestManager_ {
  private:    
    static LinkedList<Request> requestList;
    static RequestDecoder decoder;
    static ResponseEncoder encoder;

  public:
    static Buffer<uint8_t> &requestBuffer;
    static Buffer<uint8_t> &responseBuffer;

    void init(Buffer<uint8_t> &requestBuffer, Buffer<uint8_t> &responseBuffer);    

    void cycle();

  private:
    RequestManager_() {}

    // @brief: Links request to handler.
    handlerFunc linkRequest(REQ_TYPE);
    // @breif: "Executes" all requests in the request list.
    int16_t executeRequests();

    // @brief: Removes persistant request from list.
    Error removeRequest();
    // @brief: Changes the property of a request.
    Error changeProperty();
    // @brief: Syncs the request list with the host.
    Error syncRequests();
};
extern RequestManager_ &RequestManager;

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Request Handlers
///////////////////////////////////////////////////////////////////////////////////////////////////

// Req. Handler Template: void myHandler(Buffer<uint8_t>&, Request&);

void I2C_writeRegister(Request &req);

void I2C_readRegister(Request &req);

void I2C_scanBus(Request &req);

// More to add...
