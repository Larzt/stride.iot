#pragma once

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "stride_logger.hpp"

#define SD_HOST    SPI3_HOST
#define SD_MOSI    GPIO_NUM_23
#define SD_MISO    GPIO_NUM_19
#define SD_CLK     GPIO_NUM_18
#define SD_CS      GPIO_NUM_5

#define TFT_HOST   SPI2_HOST
#define TFT_RST    4
#define TFT_RS     2
#define TFT_CS     15
#define TFT_SDI    13
#define TFT_CLK    14

void spi_sd_init();
