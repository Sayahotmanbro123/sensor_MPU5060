#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main() {
  bool error = true;
  if (error) {
    printf("Error occurred!\n");
  }
  else {
    printf("Hello, Wokwi!\n");
  }
  printf("Hello, Wokwi!\n");
  while (true) {
    printf("Hello, Wokwi!\n");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}