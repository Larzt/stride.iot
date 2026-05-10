#pragma once

#include <stdio.h>
#include "driver/i2c_master.h"
#include "esp_err.h"

#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_NUM I2C_NUM_0
#define PCF8574_ADDR 0x27

esp_err_t i2c_master_init(void);
esp_err_t pcf8574_write(uint8_t data);
