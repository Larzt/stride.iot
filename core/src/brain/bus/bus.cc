#include "bus.hpp"

void spi_sd_init()
{
  spi_bus_config_t bus_cfg = {};
  bus_cfg.mosi_io_num = SD_MOSI;
  bus_cfg.miso_io_num = SD_MISO;
  bus_cfg.sclk_io_num = SD_CLK;
  bus_cfg.quadwp_io_num = -1;
  bus_cfg.quadhd_io_num = -1;
  bus_cfg.max_transfer_sz = 4096;

  esp_err_t ret = spi_bus_initialize(SD_HOST, &bus_cfg, SPI_DMA_CH_AUTO);

  if (ret == ESP_ERR_INVALID_STATE)
  {
    StrideLogger::Warning(StrideSubsystem::Card, "SPI3 bus already initialized (ok)");
  }
  else
  {
    ESP_ERROR_CHECK(ret);
  }
}
