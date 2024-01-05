
#include <Arduino.h>
#include <DMA_Util.h>
#include <GlobalDefs.h>

volatile bool readFlag = false;
volatile int16_t irReason = 5;

void callback(DMA_INTERRUPT_REASON reason, int16_t currentBlock, DMAChannel &Channel) {
  readFlag = true;
  irReason = (int16_t)reason;
}

uint8_t sourceArray[] = { 1, 2, 3, 4, 5}, destinationArray[] = { 0, 0, 0, 0, 0 }, destinationArray1[] = { 0, 0, 0, 0, 0 };


void printArray(uint8_t *array, int16_t size) {
  for (int i = 0; i < size; i++) {
    if (i % 10 == 0) {
      Serial.println();
    }
    Serial.print(array[i]);
    Serial.print(" ");
  }
  Serial.println();
}

void setup() {
  Serial.begin(9600);
  while(!Serial) {
    delay(10);
  }
  Serial.println("CONNECTED");

  DMASettings mysettings;
  mysettings
    .setBurstSize(1)
    .setPriorityLevel(PRIORITY_HIGHEST)
    .setTransferMode(BLOCK_MODE)
    .setTransferThreshold(1)
    .setCallbackFunction(callback);

  DMATask task1;
  task1
    .setElementSize(1)
    .setTransferAmount(5)
    .setIncrementConfig(true, true)
    .setSource(&sourceArray)
    .setDestination(&destinationArray);

  DMATask task2;
  task2 
    .setElementSize(1)
    .setTransferAmount(5)
    .setIncrementConfig(true, true)
    .setSource(&sourceArray)
    .setDestination(&destinationArray1);

  DMA.begin();

  DMA[0].init();
  DMATask *taskArray[] = { &task1, &task2 }; 
  DMA[0].setTasks(taskArray, 2);

  DMA[0].start();
  while(DMA[0].isBusy());  

  DMA[0].start();
  while(DMA[0].isBusy()); 

  printDesc(0);

  Serial.println(irReason);
  printArray(sourceArray, sizeof(sourceArray));
  printArray(destinationArray, sizeof(destinationArray));
  printArray(destinationArray1, sizeof(destinationArray));
  


  Serial.println("------ TEST COMPLETE ------");
}

void loop() {

}

///////////////////////////////////////////////////////////////////////////////////////////////////
