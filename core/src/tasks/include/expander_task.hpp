#pragma once
#include "i2c_bus.hpp"
#include "stride_logger.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

esp_err_t expander_init(void);
esp_err_t expander_write(uint8_t data);
void expander_task(void *pvParameters);
