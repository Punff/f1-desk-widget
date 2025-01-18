#include <LittleFS.h>
#include "FS.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <PNGdec.h>
#include "SPI.h"
#include <TFT_eSPI.h>

void setup() {
  Serial.begin(9600);
  
  //formatLittleFS();
  setupFS();
  setupWiFi();
  waitForConnection();
  updateData();
  loadData();
  setupRotaryEncoder();
  setupDisplay();
  Serial.println("Initialisation done.");
  hardwareInfo();
  handleMenu();
}

void loop() {
  handleDisplay();
  checkDailyUpdate();
}

void hardwareInfo() {
  uint32_t flashSize = ESP.getFlashChipSize();
  uint32_t flashFreeSpace = ESP.getFreeSketchSpace();
  float freeFlashSpacePercentage = (float)flashFreeSpace / flashSize * 100;
  float freeFlashSpaceKB = flashFreeSpace / 1024.0;
  float totalFlashSizeKB = flashSize / 1024.0;

  uint32_t freeHeap = ESP.getFreeHeap();
  uint32_t totalHeap = ESP.getHeapSize();
  float freeHeapPercentage = (float)freeHeap / totalHeap * 100;
  float freeHeapKB = freeHeap / 1024.0;
  float totalHeapKB = totalHeap / 1024.0;

  Serial.printf("Free Flash Space: %.2f KB\n", freeFlashSpaceKB);
  Serial.printf("Free Flash Space Percentage: %.2f %%\n", freeFlashSpacePercentage);
  Serial.printf("Total Flash Size: %.2f KB\n", totalFlashSizeKB);

  Serial.printf("Total RAM Size: %.2f KB\n", totalHeapKB);
  Serial.printf("Free RAM (Heap): %.2f KB\n", freeHeapKB);
  Serial.printf("Free RAM Percentage: %.2f %%\n", freeHeapPercentage);
}
