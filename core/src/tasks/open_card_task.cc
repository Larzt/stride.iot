#include "card_task.hpp"
#include "bus.hpp"

void open_card_task(void *pvParameters)
{
    StrideLogger::Log(StrideSubsystem::Card, "Initializing SD card");

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SD_HOST;

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_CS;
    slot_config.host_id = SD_HOST;

    esp_vfs_fat_mount_config_t mount_config = {};
    mount_config.format_if_mount_failed = false;
    mount_config.max_files = 5;
    mount_config.allocation_unit_size = 16 * 1024;

    sdmmc_card_t *card;
    esp_err_t ret = esp_vfs_fat_sdspi_mount(
        Blackboard::MountPoint.c_str(),
        &host,
        &slot_config,
        &mount_config,
        &card);

    if (ret != ESP_OK)
    {
        StrideLogger::Error(StrideSubsystem::Card, "Failed to mount SD card");
        vTaskDelete(NULL);
        return;
    }

    StrideLogger::Log(StrideSubsystem::Card, "SD card mounted successfully");

    if (sdReadTaskHandle)
        xTaskNotifyGive(sdReadTaskHandle);

    vTaskDelete(NULL);
}
