#pragma once
#include "stride_base.hpp"
#include "driver/gpio.h"

class StrideLed : public StrideBase
{

public:
  explicit StrideLed(gpio_num_t pin, bool state = false, bool active_low = false)
      : StrideBase(pin), _state(state), _active_low(active_low) {
        start();
      }

  void start() override;

  void on();
  void off();
  void toggle();
  void set(bool state); // true = ON, false = OFF
  bool get() const { return _state; }

private:
  void apply();

  bool _state;
  bool _active_low;
};
