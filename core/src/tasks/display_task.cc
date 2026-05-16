#include "display_task.hpp"
#include "startup_state.hpp"
#include "network.hpp"
#include "stride_locator.hpp"

void display_task(void *pvParameters)
{
  StrideButton left_button(Blackboard::LeftButton);
  StrideButton right_button(Blackboard::RightButton);
  StrideButton enter_button(Blackboard::EnterButton);

  Display::Instance().transition_to(std::make_unique<StartupState>());

  TickType_t combo_held_since = 0;
  bool combo_fired = false;

  while (true)
  {
    Display::Instance().update();
    auto state = Display::Instance().get_current_state();

    bool both_held = left_button.is_pressed() && right_button.is_pressed();
    if (both_held)
    {
      if (combo_held_since == 0)
      {
        combo_held_since = xTaskGetTickCount();
      }

      uint32_t held_ms = (xTaskGetTickCount() - combo_held_since) * portTICK_PERIOD_MS;
      if (!combo_fired && held_ms >= (uint32_t)Blackboard::ReconnectHoldMs)
      {
        combo_fired = true;
        StrideLogger::Log(StrideSubsystem::Network, "Combo left+right: reconnecting WiFi");
        auto *net = StrideLocator::Get<class Network>();
        if (net)
        {
          net->reconnect();
        }
      }
    }
    else
    {
      combo_held_since = 0;
      combo_fired = false;
    }

    if (!both_held)
    {
      InputEvent event;
      event.type = InputType::Button;

      if (left_button.just_pressed())
      {
        StrideLogger::Log(StrideSubsystem::Screen, "Pressed left button");
        event.delta = -1;
        state->on_input(event);
      }

      if (right_button.just_pressed())
      {
        StrideLogger::Log(StrideSubsystem::Screen, "Pressed right button");
        event.delta = 1;
        state->on_input(event);
      }

      if (enter_button.just_pressed())
      {
        event.delta = 0;
        StrideLogger::Log(StrideSubsystem::Screen, "Pressed enter button");
        state->on_input(event);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}
