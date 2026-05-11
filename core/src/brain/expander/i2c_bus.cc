#include "i2c_bus.hpp"

static i2c_master_bus_handle_t bus_handle = nullptr;

esp_err_t i2c_master_init(void)
{
    i2c_master_bus_config_t bus_conf = {};
    bus_conf.i2c_port = I2C_MASTER_NUM;
    bus_conf.sda_io_num = (gpio_num_t)I2C_MASTER_SDA_IO;
    bus_conf.scl_io_num = (gpio_num_t)I2C_MASTER_SCL_IO;
    bus_conf.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_conf.glitch_ignore_cnt = 7;
    bus_conf.flags.enable_internal_pullup = true;

    return i2c_new_master_bus(&bus_conf, &bus_handle);
}

i2c_master_bus_handle_t i2c_get_bus(void)
{
    return bus_handle;
}
