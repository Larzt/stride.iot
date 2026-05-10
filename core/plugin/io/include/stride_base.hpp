#pragma once
#include "driver/gpio.h"

class StrideBase
{
public:
  StrideBase(gpio_num_t pin) : _pin(pin) {};
  virtual void start() = 0;
  virtual ~StrideBase() = default;

protected:
  gpio_num_t _pin;
  gpio_num_t get_pin() const { return _pin; }
};
