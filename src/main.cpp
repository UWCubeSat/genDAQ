///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Pre-Processor & General Declarations
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include <Containers.h>
#include <RequestSys.h>
#include <Communication.h> 

const int16_t REQUEST_MAX_SIZE = 256;
const int16_t RESPONSE_MAX_SIZE = 512;

void cycleProgram();

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Setup & Loop Functions
///////////////////////////////////////////////////////////////////////////////////////////////////


void setup() {
  /*
  Serial.begin(0);
  while(!Serial) {
    delay(20);
  }
  */
}

void loop() {

}

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Cycle Program
///////////////////////////////////////////////////////////////////////////////////////////////////

void cycleProgram() {
  static uint8_t requestArray[REQUEST_MAX_SIZE] = { 0 };
  static uint8_t responseArray[RESPONSE_MAX_SIZE] = { 0 };
  static Buffer<uint8_t> requestMessage(requestArray, REQUEST_MAX_SIZE);
  static Buffer<uint8_t> responseMessage(responseArray, RESPONSE_MAX_SIZE);

  // Controll program flow here.
  // 1. Use communication utilities to get communication
  // 2. Update requests (request manager)
  // 3. Execute requests (request manager) -> thus filling response buffer.
  // 4. Use communication utilities to send response.
} 





