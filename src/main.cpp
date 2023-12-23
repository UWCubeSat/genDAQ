#include <Arduino.h>
#include "Global_Utilities.h"
#include "Hardware_Utilities.h"

void setup() {
  Serial.begin(0);
  while(!Serial) {
    delay(20);
  }
}

void loop() {

}

