#pragma once
#include "i2c_bus.hpp"
#include "stride_logger.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

esp_err_t expander_init(void);
esp_err_t expander_write(uint8_t data);
esp_err_t expander_pin_write(uint8_t pin, bool high);
esp_err_t expander_pin_read(uint8_t pin, bool &out);
void expander_task(void *pvParameters);
