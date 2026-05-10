#include "i2c_bus.hpp"

// Variables estáticas para mantener la referencia al bus y al dispositivo
static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t pcf_dev_handle;

esp_err_t i2c_master_init(void)
{
    i2c_master_bus_config_t bus_conf = {};
    bus_conf.i2c_port = I2C_MASTER_NUM;
    bus_conf.sda_io_num = (gpio_num_t)I2C_MASTER_SDA_IO;
    bus_conf.scl_io_num = (gpio_num_t)I2C_MASTER_SCL_IO;
    bus_conf.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_conf.glitch_ignore_cnt = 7;
    bus_conf.flags.enable_internal_pullup = true;

    esp_err_t err = i2c_new_master_bus(&bus_conf, &bus_handle);
    if (err != ESP_OK) return err;

    i2c_device_config_t dev_conf = {};
    dev_conf.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_conf.device_address = PCF8574_ADDR;
    dev_conf.scl_speed_hz = 100000;

    return i2c_master_bus_add_device(bus_handle, &dev_conf, &pcf_dev_handle);
}

esp_err_t pcf8574_write(uint8_t data)
{
    // Enviamos 1 byte de datos. El tiempo de espera (-1) es infinito (bloqueante)
    return i2c_master_transmit(pcf_dev_handle, &data, 1, -1);
}
