#include "card_task.hpp"

#include "lexer.hpp"

#include "interpreter.hpp"
#include "types.hpp"

#include <vector>

TaskHandle_t sdReadTaskHandle = NULL;
void read_card_task(void *pvParameters)
{
  auto &interpreter = Interpreter::Instance();
  StrideProgram program;

  while (true)
  {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    interpreter.stop_endless_loop();


    std::string current_program = Blackboard::MountPoint + Blackboard::CurrentLoadProgramFile.get();
    FILE *f = fopen(current_program.c_str(), "r");
    if (!f)
    {
      StrideLogger::Warning(StrideSubsystem::Card, "Could not load program file: %s", current_program.c_str());
      continue;
    }

    StrideLogger::Log(StrideSubsystem::Card, "Loading program file: %s", current_program.c_str());
    program.clear();

    char line[128];
    while (fgets(line, sizeof(line), f))
    {
      std::string s_line(line);
      s_line.erase(s_line.find_last_not_of("\n\r\t ") + 1);

      if (s_line.empty()) continue;

      auto tokens = tokenize(s_line);
      if (!tokens.empty())
      {
        program.push_back(tokens);
      }
    }

    fclose(f);

    StrideLogger::Log(StrideSubsystem::Card, "Program loadded succesfully (%d lines)", program.size());

    interpreter.execute(program);
  }
}
