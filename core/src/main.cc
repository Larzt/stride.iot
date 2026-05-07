// #include "stride_logger.hpp"
// #include "network.hpp"
// #include "server.hpp"

// #include "server_mode_task.hpp"
// #include "select_task.hpp"
// #include "card_task.hpp"
// #include "display_task.hpp"
// #include "i2c_task.hpp"

// extern "C" void app_main(void)
// {
//     ESP_ERROR_CHECK(i2c_master_init());

//     class Network network;
//     network.connect();

//     class Server server;
//     server.start_server();

//     StrideLed server_mode_led(GPIO_NUM_26, true);
//     Blackboard::CurrentServerMode.subscribe([&server_mode_led](const auto &mode)
//                                             {
//     StrideLogger::Log(StrideSubsystem::Server, "Server changed mode");
//     server_mode_led.toggle(); });

//     xTaskCreatePinnedToCore(
//         hear_server_mode_button_task,
//         "HearServerModeButton",
//         4096,
//         NULL,
//         5,
//         NULL,
//         1);

//     xTaskCreatePinnedToCore(
//         hear_program_selected_file_button_task,
//         "HearServerModeButton",
//         4096,
//         NULL,
//         5,
//         NULL,
//         1);

//     xTaskCreatePinnedToCore(
//         open_card_task,
//         "OpenCard",
//         4096,
//         NULL,
//         5,
//         NULL,
//         1);

//     xTaskCreatePinnedToCore(
//         read_card_task,
//         "OpenCard",
//         4096,
//         NULL,
//         5,
//         &sdReadTaskHandle,
//         1);

//     xTaskCreatePinnedToCore(
//         display_task,
//         "DisplayTFT",
//         4096,
//         NULL,
//         5,
//         NULL,
//         1);

//     xTaskCreatePinnedToCore(
//         pcf8574_task,
//         "Expand",
//         4096,
//         NULL,
//         5,
//         NULL,
//         0);

//     while (true)
//     {
//         vTaskDelay(pdMS_TO_TICKS(1000));
//     }
// }

#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"

#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 100000

#define MPU6050_ADDR 0x68
#define MPU6050_PWR_MGMT_1 0x6B
#define MPU6050_ACCEL_XOUT_H 0x3B

static const char *TAG = "MPU6050";

esp_err_t i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(I2C_MASTER_NUM, &conf);

    return i2c_driver_install(
        I2C_MASTER_NUM,
        conf.mode,
        0,
        0,
        0);
}

esp_err_t mpu6050_write_byte(uint8_t reg, uint8_t data)
{
    uint8_t write_buf[2] = {reg, data};

    return i2c_master_write_to_device(
        I2C_MASTER_NUM,
        MPU6050_ADDR,
        write_buf,
        sizeof(write_buf),
        pdMS_TO_TICKS(1000));
}

esp_err_t mpu6050_read(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(
        I2C_MASTER_NUM,
        MPU6050_ADDR,
        &reg,
        1,
        data,
        len,
        pdMS_TO_TICKS(1000));
}

void mpu6050_init(void)
{
    // Despertar MPU6050
    mpu6050_write_byte(MPU6050_PWR_MGMT_1, 0x00);
}

void app_main(void)
{
    ESP_ERROR_CHECK(i2c_master_init());

    mpu6050_init();

    uint8_t data[14];

    while (1)
    {
        if (mpu6050_read(MPU6050_ACCEL_XOUT_H, data, 14) == ESP_OK)
        {
            int16_t accel_x = (data[0] << 8) | data[1];
            int16_t accel_y = (data[2] << 8) | data[3];
            int16_t accel_z = (data[4] << 8) | data[5];

            int16_t gyro_x = (data[8] << 8) | data[9];
            int16_t gyro_y = (data[10] << 8) | data[11];
            int16_t gyro_z = (data[12] << 8) | data[13];

            ESP_LOGI(TAG,
                     "ACCEL X:%d Y:%d Z:%d | GYRO X:%d Y:%d Z:%d",
                     accel_x, accel_y, accel_z,
                     gyro_x, gyro_y, gyro_z);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
