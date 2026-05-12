#pragma once
#include <vector>
#include <string>
#include <dirent.h>
#include <memory>

#include "blackboard.hpp"
#include "types.hpp"
#include "bus.hpp"

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

enum TFT_DIRECTION
{
  Vertical = 0,
  Horizontal = 1,
  InvertedVertical = 2,
  InvertedHorizontal = 3,
};

class LGFX_Config : public lgfx::LGFX_Device
{
  lgfx::Panel_ILI9225 _panel_instance;
  lgfx::Bus_SPI _bus_instance;

public:
  LGFX_Config();
};

class Display;
class DisplayBaseState
{
public:
  virtual ~DisplayBaseState() = default;
  virtual void on_enter(Display &ctx) = 0;
  virtual void on_update(Display &ctx) = 0;
  virtual void on_exit(Display &ctx) = 0;
  virtual StateType get_type() const = 0;
  virtual void on_input(const InputEvent &event) {}
};

class Display
{
public:
  static Display &Instance()
  {
    static Display instance;
    return instance;
  }

  void begin();
  void transition_to(std::unique_ptr<DisplayBaseState> newState);
  void update();

  LGFX_Config &getTFT() { return _tft; }
  DisplayBaseState *get_current_state() { return _current_state.get(); }

private:
  Display() = default;

  LGFX_Config _tft;
  std::unique_ptr<DisplayBaseState> _current_state;
};
