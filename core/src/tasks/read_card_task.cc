#include "card_task.hpp"

#include "lang_parser.hpp"

#include "interpreter.hpp"
#include "types.hpp"
#include "app.hpp"
#include "sink.hpp"
#include "timer.hpp"

#include <string>

// Reports every parse error to the logger and to the active log file, so the
// user can read them from the web (/view) or the TFT.
static void report_parse_errors(const std::vector<lang::ParseError> &errors)
{
  std::string log_path = Blackboard::MountPoint + Blackboard::CurrentLogFile;
  std::string timestamp = TimeUtils::get_timestamp();

  sink_file(log_path, "[" + timestamp + "]: El programa tiene errores y no se ejecuto:\n");

  for (const auto &error : errors)
  {
    StrideLogger::Error(StrideSubsystem::Interpreter, "Linea %u: %s",
                        (unsigned)error.line, error.message.c_str());
    sink_file(log_path, "  - Linea " + std::to_string(error.line) + ": " +
                            error.message + "\n");
  }
}

TaskHandle_t sdReadTaskHandle = NULL;
void read_card_task(void *pvParameters)
{
  auto &interpreter = Interpreter::Instance();

  while (true)
  {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    interpreter.stop_endless_loop();

    AppDescriptor app = Blackboard::CurrentProgram.get();
    if (app.name.empty()) {
      continue;
    }

    std::string current_program = Blackboard::MountPoint + app.path;
    FILE *f = fopen(current_program.c_str(), "r");
    if (!f)
    {
      StrideLogger::Warning(StrideSubsystem::Card, "Could not load program file: %s", current_program.c_str());
      continue;
    }

    StrideLogger::Log(StrideSubsystem::Card, "Loading program file: %s", current_program.c_str());

    // Whole-file read: the parser needs the complete source (and the old
    // 128-char line buffer silently truncated long lines).
    std::string source;
    char chunk[256];
    size_t bytes_read;
    while ((bytes_read = fread(chunk, 1, sizeof(chunk), f)) > 0)
      source.append(chunk, bytes_read);

    fclose(f);

    lang::ParseResult result = lang::parse(source);

    if (!result.ok())
    {
      StrideLogger::Error(StrideSubsystem::Card,
                          "Program '%s' has %u error(s), not running",
                          app.name.c_str(), (unsigned)result.errors.size());
      report_parse_errors(result.errors);
      continue;
    }

    StrideLogger::Log(StrideSubsystem::Card,
                      "Program parsed successfully (%u statements)",
                      (unsigned)result.program.top.size());

    Blackboard::RunningProgramName = app.name;
    interpreter.execute(result.program);
    Blackboard::RunningProgramName = std::string("");
  }
}
