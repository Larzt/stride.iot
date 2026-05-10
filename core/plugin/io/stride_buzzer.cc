#include "stride_buzzer.hpp"


void StrideBuzzer::start()
{
  gpio_reset_pin(_pin);
  gpio_set_direction(_pin, GPIO_MODE_OUTPUT);

  apply();
}

void StrideBuzzer::on()
{
  _state = true;
  apply();
}

void StrideBuzzer::off()
{
  _state = false;
  apply();
}

void StrideBuzzer::toggle()
{
  _state = !_state;
  apply();
}

void StrideBuzzer::set(bool state)
{
  _state = state;
  apply();
}

void StrideBuzzer::apply()
{
  int level = _active_low ? !_state : _state;
  gpio_set_level(_pin, level);
}
