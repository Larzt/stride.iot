#include "expander_task.hpp"

#define PCF8574_ADDR 0x27
static i2c_master_dev_handle_t pcf_dev;

esp_err_t expander_init(void)
{
  i2c_device_config_t cfg = {};
  cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
  cfg.device_address = PCF8574_ADDR;
  cfg.scl_speed_hz = 100000;

  return i2c_master_bus_add_device(
      i2c_get_bus(),
      &cfg,
      &pcf_dev);
}

esp_err_t expander_write(uint8_t data)
{
  return i2c_master_transmit(pcf_dev, &data, 1, -1);
}

void expander_task(void *pvParameters)
{
  StrideLogger::Log(StrideSubsystem::Expander, "Tarea del expansor iniciada en el núcleo: %d\n", xPortGetCoreID());
  while (1)
  {
    // Encender LED P0
    expander_write(0xFE);
    vTaskDelay(pdMS_TO_TICKS(500));

    // Apagar LED P0
    expander_write(0xFF);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}
