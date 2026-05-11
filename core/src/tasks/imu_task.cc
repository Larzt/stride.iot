#include "imu_task.hpp"
#include <cmath>
#include "stride_logger.hpp"
#include "blackboard.hpp"
#include "i2c_bus.hpp"

#define MPU_ADDR 0x68
#define ACCEL_XOUT 0x3B
#define PWR_MGMT_1 0x6B

static const float ACCEL_SCALE = 16384.0f;
static const float GYRO_SCALE = 131.0f;
static const float RAD_TO_DEG = 57.2958f;

static i2c_master_dev_handle_t imu_dev;

esp_err_t imu_init(void)
{
  i2c_device_config_t cfg = {};
  cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
  cfg.device_address = MPU_ADDR;
  cfg.scl_speed_hz = 400000;

  return i2c_master_bus_add_device(
      i2c_get_bus(),
      &cfg,
      &imu_dev);
}

void imu_task(void *pvParameters)
{
  float roll = 0, pitch = 0, yaw = 0;
  float gx_off = 0, gy_off = 0, gz_off = 0;

  const float dt = 0.01f;
  const float alpha = 0.98f;
  uint8_t data[14];

  uint8_t reg = ACCEL_XOUT;

  // calibración
  for (int i = 0; i < 300; i++)
  {
    i2c_master_transmit_receive(imu_dev, &reg, 1, data, 14, -1);

    gx_off += (int16_t)((data[8] << 8) | data[9]);
    gy_off += (int16_t)((data[10] << 8) | data[11]);
    gz_off += (int16_t)((data[12] << 8) | data[13]);

    vTaskDelay(pdMS_TO_TICKS(5));
  }

  gx_off /= 300.0f;
  gy_off /= 300.0f;
  gz_off /= 300.0f;

  while (true)
  {
    if (i2c_master_transmit_receive(imu_dev, &reg, 1, data, 14, -1) == ESP_OK)
    {
      int16_t ax = (data[0] << 8) | data[1];
      int16_t ay = (data[2] << 8) | data[3];
      int16_t az = (data[4] << 8) | data[5];

      int16_t gx = (data[8] << 8) | data[9];
      int16_t gy = (data[10] << 8) | data[11];
      int16_t gz = (data[12] << 8) | data[13];

      float axg = ax / ACCEL_SCALE;
      float ayg = ay / ACCEL_SCALE;
      float azg = az / ACCEL_SCALE;

      float gxd = (gx - gx_off) / GYRO_SCALE;
      float gyd = (gy - gy_off) / GYRO_SCALE;
      float gzd = (gz - gz_off) / GYRO_SCALE;

      float roll_acc = atan2(ayg, azg) * RAD_TO_DEG;
      float pitch_acc = atan2(-axg, sqrt(ayg * ayg + azg * azg)) * RAD_TO_DEG;

      roll += gxd * dt;
      pitch += gyd * dt;
      yaw += gzd * dt;

      roll = alpha * roll + (1.0f - alpha) * roll_acc;
      pitch = alpha * pitch + (1.0f - alpha) * pitch_acc;

      // Blackboard::ImuRoll = roll;
      // Blackboard::ImuPitch = pitch;
      // Blackboard::ImuYaw = yaw;
      StrideLogger::Log(StrideSubsystem::IMU, "R: %.2f | P: %.2f | Y: %.2f", roll, pitch, yaw);
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
