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
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2c_master.h"
#include "esp_log.h"

#define SDA GPIO_NUM_21
#define SCL GPIO_NUM_22
#define I2C_PORT I2C_NUM_0
#define MPU_ADDR 0x68

#define ACCEL_XOUT 0x3B
#define PWR_MGMT_1 0x6B

static const char *TAG = "IMU";

static i2c_master_bus_handle_t bus;
static i2c_master_dev_handle_t dev;

// Estado orientación
float roll = 0;
float pitch = 0;
float yaw = 0;

// bias gyro (calibración simple)
float gx_off = 0, gy_off = 0, gz_off = 0;

void i2c_init()
{
    i2c_master_bus_config_t bus_cfg = {};
    bus_cfg.i2c_port = I2C_PORT;
    bus_cfg.sda_io_num = SDA;
    bus_cfg.scl_io_num = SCL;
    bus_cfg.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_cfg.glitch_ignore_cnt = 7;

    i2c_new_master_bus(&bus_cfg, &bus);

    i2c_device_config_t dev_cfg = {};
    dev_cfg.device_address = MPU_ADDR;
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.scl_speed_hz = 100000;

    i2c_master_bus_add_device(bus, &dev_cfg, &dev);
}

void mpu_write(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    i2c_master_transmit(dev, buf, 2, -1);
}

void mpu_read(uint8_t reg, uint8_t *buf, size_t len)
{
    i2c_master_transmit_receive(dev, &reg, 1, buf, len, -1);
}

// calibración rápida
void calibrate()
{
    uint8_t d[14];

    for (int i = 0; i < 300; i++) {
        mpu_read(ACCEL_XOUT, d, 14);

        int16_t gx = (d[8] << 8) | d[9];
        int16_t gy = (d[10] << 8) | d[11];
        int16_t gz = (d[12] << 8) | d[13];

        gx_off += gx;
        gy_off += gy;
        gz_off += gz;

        vTaskDelay(pdMS_TO_TICKS(5));
    }

    gx_off /= 300;
    gy_off /= 300;
    gz_off /= 300;
}

extern "C" void app_main()
{
    i2c_init();

    mpu_write(PWR_MGMT_1, 0x00);

    calibrate();

    uint8_t d[14];

    const float dt = 0.01;   // 100 Hz
    const float alpha = 0.98;

    while (1) {

        mpu_read(ACCEL_XOUT, d, 14);

        // acelerómetro
        int16_t ax = (d[0] << 8) | d[1];
        int16_t ay = (d[2] << 8) | d[3];
        int16_t az = (d[4] << 8) | d[5];

        // gyro
        int16_t gx = (d[8] << 8) | d[9];
        int16_t gy = (d[10] << 8) | d[11];
        int16_t gz = (d[12] << 8) | d[13];

        float axg = ax / 16384.0f;
        float ayg = ay / 16384.0f;
        float azg = az / 16384.0f;

        float gxd = (gx - gx_off) / 131.0f;
        float gyd = (gy - gy_off) / 131.0f;
        float gzd = (gz - gz_off) / 131.0f;

        // roll/pitch desde acelerómetro
        float roll_acc  = atan2(ayg, azg) * 57.2958f;
        float pitch_acc = atan2(-axg, sqrt(ayg*ayg + azg*azg)) * 57.2958f;

        // integración gyro
        roll  += gxd * dt;
        pitch += gyd * dt;
        yaw   += gzd * dt;

        // filtro complementario
        roll  = alpha * roll  + (1 - alpha) * roll_acc;
        pitch = alpha * pitch + (1 - alpha) * pitch_acc;

        ESP_LOGI(TAG,
            "ROLL: %.2f PITCH: %.2f YAW: %.2f",
            roll, pitch, yaw);

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
