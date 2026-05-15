#include "select_task.hpp"
#include "display.hpp"
#include "startup_state.hpp"
#include "view_state.hpp"

bool is_executable_program(const std::string &file)
{
  if (file.length() < 4)
    return false;

  std::string ext = file.substr(file.length() - 4);

  return (ext == ".str" || ext == ".STR");
}

void hear_program_selected_file_button_task(void *pvParameters)
{
  StrideButton button(Blackboard::EnterButton);

  while (true)
  {
    if (button.wait_for_long_press(Blackboard::ReloadTimePressed))
    {
      AppDescriptor app = Blackboard::CurrentProgram.get();
      std::string file = app.path;
      if (app.type == AppType::Script)
      {
        if (!is_executable_program(file))
        {
          Display::Instance().transition_to(std::make_unique<ViewState>(file));
          vTaskDelay(pdMS_TO_TICKS(10));
          continue;
        }
      }
      else
      {
        StrideLogger::Error(StrideSubsystem::Interpreter, "Loading selected program: %s has unknown type.", file.c_str());
        vTaskDelay(pdMS_TO_TICKS(10));
        continue;
      }

      StrideLogger::Log(StrideSubsystem::Interpreter, "Loading selected program: %s", file.c_str());

      Interpreter::Instance().stop_endless_loop();

      if (sdReadTaskHandle != NULL)
      {
        xTaskNotifyGive(sdReadTaskHandle);
      }

      while (button.is_pressed())
      {
        vTaskDelay(pdMS_TO_TICKS(100));
      }
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
