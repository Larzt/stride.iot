#include "display_task.hpp"
#include "startup_state.hpp"

void display_task(void *pvParameters)
{
  StrideButton left_button(Blackboard::LeftButton);
  StrideButton right_button(Blackboard::RightButton);
  StrideButton enter_button(Blackboard::EnterButton);

  Display::Instance().begin();
  Display::Instance().transition_to(std::make_unique<StartupState>());

  while (true)
  {
    Display::Instance().update();
    auto state = Display::Instance().get_current_state();

    InputEvent event;
    event.type = InputType::Button;

    if (left_button.just_pressed())
    {
      event.delta = -1;
      state->on_input(event);
    }

    if (right_button.just_pressed())
    {
      event.delta = 1;
      state->on_input(event);
    }

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}
