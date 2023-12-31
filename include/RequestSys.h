///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Pre-Processor & General Declarations
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Arduino.h"
#include <Containers.h>
#include <Hardware.h>
#include <GlobalDefs.h>

#define REQ_PARAM_ARRAY_BYTES 64

struct Request;

typedef void (*reqHandler)(Request*);  // Ptr to request handler function

enum REQ_TYPE : uint8_t {
  REQ_NULL = 0,
  REQ_I2C_WRITE = 1
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Request Object
///////////////////////////////////////////////////////////////////////////////////////////////////

struct Request {
    uint8_t requestType;
    uint8_t id = 0;
    reqHandler handler = nullptr;
    uint8_t *params;
    uint8_t *data; 


    // #Default Constructor
    Request() {}

    // #Constructor
    // @brief: Sets the properties of the request.
    Request(REQ_TYPE, uint8_t, reqHandler, uint8_t*, uint8_t*);

    // @brief: Resets all fields.
    void clear();
};

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
    //static RequestDecoder decoder;
    //static ResponseEncoder encoder;

  public:
    static Buffer<uint8_t> &requestBuffer;
    static Buffer<uint8_t> &responseBuffer;

    void init(Buffer<uint8_t> &requestBuffer, Buffer<uint8_t> &responseBuffer);    

    void cycle();

  private:
    RequestManager_() {}

    // @brief: Links request to handler.
    reqHandler linkRequest(REQ_TYPE);
    // @breif: "Executes" all requests in the request list.
    int16_t executeRequests();

    // @brief: Removes persistant request from list.
    void removeRequest();
    // @brief: Changes the property of a request.
    void changeProperty();
    // @brief: Syncs the request list with the host.
    void syncRequests();
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
