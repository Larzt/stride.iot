#include "server_mode_task.hpp"

void hear_server_mode_button_task(void *pvParameters)
{
  StrideButton button(Blackboard::RightButton);

  while (true)
  {

    if (button.wait_for_long_press(Blackboard::ModeTimePressed))
    {
      StrideLogger::Log(StrideSubsystem::Server, "Currently pressed for long time mode button");
      Blackboard::CurrentServerMode = Blackboard::CurrentServerMode.get() == ServerMode::Developer
      ? ServerMode::Production : ServerMode::Developer;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
