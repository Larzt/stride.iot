#include "i2c_task.hpp"

void pcf8574_task(void *pvParameters)
{
  printf("Tarea del expansor iniciada en el núcleo: %d\n", xPortGetCoreID());

  while (1)
  {
    // Encender LED P0
    pcf8574_write(0xFE);
    vTaskDelay(pdMS_TO_TICKS(500));

    // Apagar LED P0
    pcf8574_write(0xFF);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}
