#include "stride_led.hpp"


void StrideLed::start()
{
  gpio_reset_pin(_pin);
  gpio_set_direction(_pin, GPIO_MODE_OUTPUT);

  apply();
}

void StrideLed::on()
{
  _state = true;
  apply();
}

void StrideLed::off()
{
  _state = false;
  apply();
}

void StrideLed::toggle()
{
  _state = !_state;
  apply();
}

void StrideLed::set(bool state)
{
  _state = state;
  apply();
}

void StrideLed::apply()
{
  int level = _active_low ? !_state : _state;
  gpio_set_level(_pin, level);
}
