#pragma once
#include "i2c_bus.hpp"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


void pcf8574_task(void *pvParameters);
