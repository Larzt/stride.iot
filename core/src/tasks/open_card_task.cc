#include "card_task.hpp"

void open_card_task(void *pvParameters)
{
  StrideLogger::Log(StrideSubsystem::Card, "Initializing sd-card module");

  esp_err_t ret;

  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  spi_bus_config_t bus_cfg = {};
  bus_cfg.mosi_io_num = GPIO_NUM_23;
  bus_cfg.miso_io_num = GPIO_NUM_19;
  bus_cfg.sclk_io_num = GPIO_NUM_18;
  bus_cfg.quadwp_io_num = -1;
  bus_cfg.quadhd_io_num = -1;

  ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SDSPI_DEFAULT_DMA);

  if (ret == ESP_ERR_INVALID_STATE)
  {
    StrideLogger::Warning(StrideSubsystem::Card, "SPI was already initialized; continuing...");
    ret = ESP_OK;
  }

  if (ret != ESP_OK)
  {
    StrideLogger::Error(StrideSubsystem::Card, "Error initializing SPI");
    vTaskDelete(NULL);
    return;
  }

  if (ret != ESP_OK)
  {
    StrideLogger::Error(StrideSubsystem::Card, "Error initializing SPI bus");
    vTaskDelete(NULL);
    return;
  }

  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.gpio_cs = GPIO_NUM_5;
  slot_config.host_id = SPI2_HOST;

  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = 5,
      .allocation_unit_size = 16 * 1024,
      .disk_status_check_enable = true,
  };

  sdmmc_card_t *card;

  ret = esp_vfs_fat_sdspi_mount(Blackboard::MountPoint.c_str(), &host, &slot_config, &mount_config, &card);
  if (ret != ESP_OK)
  {
    ESP_LOGE("SD", "No se pudo montar la SD");
    vTaskDelete(NULL);
    return;
  }

  StrideLogger::Log(StrideSubsystem::Card, "SD mounted correctly");

  if (sdReadTaskHandle != NULL)
  {
    xTaskNotifyGive(sdReadTaskHandle);
  }

  vTaskDelete(NULL);
}
