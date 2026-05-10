#include "stride_button.hpp"

void StrideButton::start()
{
  gpio_reset_pin(_pin);
  gpio_set_direction(_pin, GPIO_MODE_INPUT);

  if (_pull_up)
  {
    gpio_set_pull_mode(_pin, GPIO_PULLUP_ONLY);
  }
  else
  {
    gpio_set_pull_mode(_pin, GPIO_PULLDOWN_ONLY);
  }
}

bool StrideButton::just_pressed()
{
  bool current_state = read_debounced();

  if (current_state && !_was_pressed)
  {
    _was_pressed = true;
    return true;
  }

  if (!current_state)
  {
    _was_pressed = false;
  }

  return false;
}

bool StrideButton::is_pressed()
{
  return read_debounced();
}

bool StrideButton::wait_for_long_press(uint32_t timeout_ms)
{
  TickType_t now = xTaskGetTickCount();
  bool pressed = read_debounced();

  if (pressed && !_is_pressing)
  {
    _is_pressing = true;
    _start_time = now;
    _last_log_time = now;
  }

  if (!pressed && _is_pressing)
  {
    _is_pressing = false;
  }

  if (_is_pressing)
  {
    if ((now - _last_log_time) >= pdMS_TO_TICKS(1000))
    {
      _last_log_time = now;
    }

    uint32_t duration_ms =
        (now - _start_time) * portTICK_PERIOD_MS;

    if (duration_ms >= timeout_ms)
    {
      _is_pressing = false;
      return true;
    }
  }

  return false;
}



bool StrideButton::read_raw()
{
  int level = gpio_get_level(_pin);
  if (_pull_up)
  {
    return (level == 0);
  }
  else
  {
    return (level == 1);
  }
}

bool StrideButton::read_debounced()
{
  bool reading = read_raw();
  TickType_t now = xTaskGetTickCount();

  if (reading != _last_reading)
  {
    _last_debounce_time = now;
  }

  if ((now - _last_debounce_time) > pdMS_TO_TICKS(DEBOUNCE_MS))
  {
    _last_state = reading;
  }

  _last_reading = reading;

  return _last_state;
}
