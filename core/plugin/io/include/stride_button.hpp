#pragma once
#include "stride_base.hpp"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class StrideButton : public StrideBase
{
public:
  explicit StrideButton(gpio_num_t pin, bool pull_up = true) : StrideBase(pin), _pull_up(pull_up)
  {
    start();
  }

  void start() override;
  bool just_pressed();
  bool is_pressed();
  bool wait_for_long_press(uint32_t ms);

private:
  bool read_raw();
  bool read_debounced();

private:
  bool _pull_up = true;
  bool _was_pressed = false;
  bool _is_pressing = false;
  bool _last_state = false;
  bool _last_reading = false;

  TickType_t _start_time = 0;
  TickType_t _last_log_time = 0;
  TickType_t _last_debounce_time = 0;

  static constexpr uint32_t DEBOUNCE_MS = 30;
};
