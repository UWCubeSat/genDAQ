#include <Arduino.h>
#include "Global_Utilities.h"
#include "Hardware_Utilities.h"

uint8_t myArray[256] = { 0 };

void setup() {
  
  Serial.begin(0);
  while(!Serial) {
    delay(20);
  }
  
 
}

void loop() {

}

