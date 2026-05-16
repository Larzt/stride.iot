#include "card_task.hpp"
#include "bus.hpp"

#include <dirent.h>

void open_card_task(void *pvParameters)
{
  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.slot = SD_HOST;

  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.gpio_cs = SD_CS;
  slot_config.host_id = SD_HOST;

  esp_vfs_fat_mount_config_t mount_config = {};
  mount_config.format_if_mount_failed = false;
  mount_config.max_files = 5;
  mount_config.allocation_unit_size = 16 * 1024;

  sdmmc_card_t *card = nullptr;

  while (true)
  {

    gpio_reset_pin(SD_CS);

    esp_err_t ret = esp_vfs_fat_sdspi_mount(
        Blackboard::MountPoint.c_str(), &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK)
    {
      StrideLogger::Warning(StrideSubsystem::Card, "SD mount failed (0x%x), retrying...", ret);
      sdspi_host_deinit();
      vTaskDelay(pdMS_TO_TICKS(5000));
      continue;
    }

    StrideLogger::Log(StrideSubsystem::Card, "SD card mounted");
    Blackboard::SdCardMounted = true;

    std::string default_log = Blackboard::MountPoint + Blackboard::CurrentLogFile;
    FILE *lf = fopen(default_log.c_str(), "a");
    if (lf)
    {
      fclose(lf);
      StrideLogger::Log(StrideSubsystem::Card, "Log file ready: %s", default_log.c_str());
    }

    Blackboard::FileListVersion = Blackboard::FileListVersion.get() + 1;

    if (sdReadTaskHandle)
      xTaskNotifyGive(sdReadTaskHandle);

    int failures = 0;
    while (true)
    {
      vTaskDelay(pdMS_TO_TICKS(5000));

      DIR *d = opendir(Blackboard::MountPoint.c_str());
      if (d)
      {
        closedir(d);
        failures = 0;
      }
      else
      {
        failures++;
        StrideLogger::Warning(StrideSubsystem::Card, "SD health check failed (%d/3)", failures);
        if (failures >= Blackboard::MaxSDRetries)
        {
          StrideLogger::Warning(StrideSubsystem::Card, "SD card removed, unmounting");
          break;
        }
      }
    }

    Blackboard::SdCardMounted = false;
    Blackboard::FileListVersion = Blackboard::FileListVersion.get() + 1;

    esp_vfs_fat_sdcard_unmount(Blackboard::MountPoint.c_str(), card);
    card = nullptr;

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
