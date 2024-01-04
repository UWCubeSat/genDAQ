
#include <Arduino.h>
#include <DMA_Util.h>

void setup() {
  Serial.begin(9600);
  while(!Serial) {
    delay(10);
  }
  Serial.println("CONNECTED");

  uint8_t sourceArray[5] = { 1, 2, 3, 4, 5};
  uint8_t destinationArray[5] = { 0, 0, 0, 0, 0 };

  DMASettings mysettings;
  mysettings
    .setBurstSize(4)
    .setPriorityLevel(PRIORITY_HIGHEST)
    .setTransferMode(BURST_MODE)
    .setTransferThreshold(3);

  DMATask task1;
  task1
    .setElementSize(2)
    .setTransferAmount(5)
    .setSource(&sourceArray)
    .setDestination(&destinationArray)
    .setIncrementConfig(true, true)
    .setIncrementModifier(x16, SOURCE);

  DMATask task2;
  task2
    .setElementSize(1)
    .setTransferAmount(3)
    .setSource(&sourceArray)
    .setDestination(&destinationArray)
    .setIncrementConfig(true, true)
    .setIncrementModifier(x16, SOURCE);

  DMATask task3;
  task3
    .setTransferAmount(7)
    .setDestination(&destinationArray)
    .setSource(&sourceArray)
    .setIncrementModifier(x128, DESTINATION);

    DMATask task4;
  task4
    .setTransferAmount(33)
    .setDestination(&destinationArray)
    .setSource(&sourceArray)
    .setIncrementModifier(x128, DESTINATION);

  DMA.begin();
  DMA[0].init(mysettings);

  DMATask *taskArray[] = { &task1, &task2, &task3 };

  DMA[0].setTasks(taskArray, 2);  
  DMA[0].addTask(&task4, 0);
  Serial.println("------ TEST COMPLETE ------");
}

void loop() {

}

///////////////////////////////////////////////////////////////////////////////////////////////////
