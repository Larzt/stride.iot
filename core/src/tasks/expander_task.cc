#include "expander_task.hpp"

#define PCF8574_ADDR 0x27
static i2c_master_dev_handle_t pcf_dev;
static uint8_t pcf_shadow = 0xFF;

esp_err_t expander_init(void)
{
  i2c_device_config_t cfg = {};
  cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
  cfg.device_address = PCF8574_ADDR;
  cfg.scl_speed_hz = 100000;

  esp_err_t err = i2c_master_bus_add_device(
      i2c_get_bus(),
      &cfg,
      &pcf_dev);

  if (err == ESP_OK)
    i2c_master_transmit(pcf_dev, &pcf_shadow, 1, pdMS_TO_TICKS(100));

  return err;
}

esp_err_t expander_write(uint8_t data)
{
  esp_err_t err = i2c_master_transmit(pcf_dev, &data, 1, -1);
  if (err == ESP_OK)
    pcf_shadow = data;
  return err;
}

esp_err_t expander_pin_write(uint8_t pin, bool high)
{
  if (pin > 7)
    return ESP_ERR_INVALID_ARG;

  uint8_t next = high ? (pcf_shadow | (1 << pin))
                      : (pcf_shadow & ~(1 << pin));

  esp_err_t err = i2c_master_transmit(pcf_dev, &next, 1, pdMS_TO_TICKS(100));
  if (err == ESP_OK)
    pcf_shadow = next;
  return err;
}

esp_err_t expander_pin_read(uint8_t pin, bool &out)
{
  if (pin > 7)
    return ESP_ERR_INVALID_ARG;

  uint8_t needed = pcf_shadow | (1 << pin);
  if (needed != pcf_shadow)
  {
    esp_err_t err = i2c_master_transmit(pcf_dev, &needed, 1, pdMS_TO_TICKS(100));
    if (err != ESP_OK)
      return err;
    pcf_shadow = needed;
  }

  uint8_t rx = 0;
  esp_err_t err = i2c_master_receive(pcf_dev, &rx, 1, pdMS_TO_TICKS(100));
  if (err != ESP_OK)
    return err;

  out = (rx >> pin) & 0x01;
  return ESP_OK;
}

void expander_task(void *pvParameters)
{
  StrideLogger::Log(StrideSubsystem::Expander, "Tarea del expansor iniciada en el núcleo: %d\n", xPortGetCoreID());
  while (1)
  {

    expander_write(0xFE);
    vTaskDelay(pdMS_TO_TICKS(500));

    expander_write(0xFF);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}
