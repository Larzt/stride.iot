#pragma once

#include "i2c_bus.hpp"
#include "stride_logger.hpp"
#include "blackboard.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"

esp_err_t imu_init(void);

void imu_enable();
void imu_disable();

void imu_task(void *pvParameters);

extern TaskHandle_t g_imu_task_handle;
