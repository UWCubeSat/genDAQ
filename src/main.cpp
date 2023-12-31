#include <Arduino.h>
#include <GlobalDefs.h>
#include <Containers.h>
#include <Utilities.h>

void setup() {
  Serial.begin(0);
  while(!Serial) {
    delay(20);
  }
  Serial.println("CONNECTED");
  Serial.println((uint8_t)System.getRestartFlag());
  System.setRestartFlag(RESTART_TEST1);
  delay(2500);
  digitalWrite(13, HIGH);
  delay(2500);
  System.forceRestart();
  /*
  pinMode(13, OUTPUT);  
  digitalWrite(13, LOW);
  delay(5000);
  digitalWrite(13, HIGH);
  */
}

void loop() {

}

