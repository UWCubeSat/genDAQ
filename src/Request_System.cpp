///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Pre-Processor & Forward Declarations
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "Request_System.h"


///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Request Manager Class
///////////////////////////////////////////////////////////////////////////////////////////////////

RequestManager_ &RequestManager;

LinkedList<Request> requestList(REQ_LIST_INIT_SIZE, REQ_LIST_MAX_SIZE, true);


void RequestManager_::init(Buffer<uint8_t> &requestbuffer, Buffer<uint8_t> &responseBuffer) {
  this->requestBuffer = requestBuffer;
  this->responseBuffer = responseBuffer;
}


void RequestManager_::cycle() {
  return;
}


handlerFunc RequestManager_::linkRequest(REQ_TYPE type) {
  return nullptr;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Request Handler Functions
///////////////////////////////////////////////////////////////////////////////////////////////////

// {bus num -> @0} {device addr -> @1} {reg addr -> @2} {num bytes -> @3} {data -> @array}
void I2C_writeRegister(Request &req) {
  int16_t writeResult = Board.I2C(req.params[0])->write(req.data, req.params[3], req.params[1], req.params[2]);
  if (writeResult < 0) {
    // !TO DO! -> REPORT ERROR
    writeResult = 0;
  }
}

void I2C_readRegister(Request &req) {

}











