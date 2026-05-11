#pragma once

#include "driver/i2c_master.h"
#include "esp_err.h"

#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_NUM I2C_NUM_0

esp_err_t i2c_master_init(void);
i2c_master_bus_handle_t i2c_get_bus(void);
