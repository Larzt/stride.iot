#pragma once
#include "stride_logger.hpp"
#include "blackboard.hpp"

#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "freertos/task.h"

extern TaskHandle_t sdReadTaskHandle;

void open_card_task(void *pvParameters);
void read_card_task(void *pvParameters);
