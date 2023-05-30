#include <Arduino.h>
#include <Wire.h>
#include <esp_log.h>
#include "LightController.h"

int firmwareVersion = 4;

void setup() {
  lcsetup(); 
}

void loop() {
  lcloop();
}

// utilities ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// delay/yield macros
#define WAIT_A_SEC   vTaskDelay(    1000/portTICK_PERIOD_MS); // 1 second
#define WAIT_SECS(n) vTaskDelay((n*1000)/portTICK_PERIOD_MS); // n seconds
#define WAIT_MS(n)   vTaskDelay(       n/portTICK_PERIOD_MS); // n millis

#define ECHECK ESP_ERROR_CHECK_WITHOUT_ABORT

// IDF logging
static const char *TAG = "main";