#include "display.hpp"
#include "startup_state.hpp"

LGFX_Config::LGFX_Config()
{
  {
    auto cfg = _bus_instance.config();
    cfg.spi_host = SPI3_HOST;
    cfg.spi_mode = 0;
    cfg.freq_write = 10000000;
    cfg.pin_sclk = TFT_CLK;
    cfg.pin_mosi = TFT_SDI;
    cfg.pin_miso = -1;
    cfg.pin_dc = TFT_RS;
    _bus_instance.config(cfg);
    _panel_instance.setBus(&_bus_instance);
  }
  {
    auto cfg = _panel_instance.config();
    cfg.pin_cs = TFT_CS;
    cfg.pin_rst = TFT_RST;
    cfg.panel_width = 176;
    cfg.panel_height = 220;
    cfg.bus_shared = false;
    _panel_instance.config(cfg);
  }
  setPanel(&_panel_instance);
}

void Display::begin()
{
  _tft.init();
  _tft.setRotation(Vertical);
  transition_to(std::make_unique<StartupState>());
}

void Display::transition_to(std::unique_ptr<DisplayBaseState> new_state)
{
  if (_current_state)
  {
    _current_state->on_exit(*this);
  }

  _current_state = std::move(new_state);

  if (_current_state)
  {
    _current_state->on_enter(*this);
  }
}

void Display::update()
{
  if (_current_state)
  {
    _current_state->on_update(*this);
  }
}
