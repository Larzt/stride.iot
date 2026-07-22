#include "interpreter.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sink.hpp"
#include "timer.hpp"

static uint32_t now_ms()
{
  return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

bool Interpreter::should_stop() const
{
  return _stop_requested || !endless_loop.load();
}

// Runtime problems are forgiving: the statement is skipped, the program goes
// on. The message lands in the logger and in the active log file so the user
// can read it from the web (/view) or the TFT.
void Interpreter::runtime_error(uint16_t line, const std::string &message)
{
  StrideLogger::Warning(StrideSubsystem::Interpreter, "Linea %u: %s",
                        (unsigned)line, message.c_str());

  std::string entry = "[" + TimeUtils::get_timestamp() + "]: [linea " +
                      std::to_string(line) + "] " + message + "\n";
  sink_file(Blackboard::MountPoint + Blackboard::CurrentLogFile, entry);
}

void Interpreter::reset_run_state()
{
  _stop_requested = false;
  start_endless_loop();

  clear_devices();
  _variables.clear();
  _when_handlers.clear();
  _every_handlers.clear();

  Blackboard::DslShowText = std::string("");
}

void Interpreter::execute(const StrideProgram &program)
{
  if (program.top.empty())
  {
    StrideLogger::Log(StrideSubsystem::Interpreter, "Program %s is empty",
                      Blackboard::CurrentProgram.get().name.c_str());
    return;
  }

  reset_run_state();

  exec_block(program.top);

  run_handlers();

  // Handlers point into `program`; drop them before returning.
  _when_handlers.clear();
  _every_handlers.clear();
}

void Interpreter::register_when(const lang::Stmt &stmt)
{
  DeviceEntry *device = find_device(stmt.name);
  if (!device || device->type != lang::DeviceType::Button)
  {
    runtime_error(stmt.line, "'" + stmt.name +
                                 "' no es un boton declarado, no puedo "
                                 "vigilarlo con 'when'");
    return;
  }

  Handler handler;
  handler.stmt = &stmt;
  handler.button = device->button;
  _when_handlers.push_back(handler);

  StrideLogger::Log(StrideSubsystem::Interpreter,
                    "Handler 'when %s %s' registrado", stmt.name.c_str(),
                    stmt.flags ? "pressed" : "released");
}

void Interpreter::register_every(const lang::Stmt &stmt)
{
  uint32_t period = resolve_duration_ms(stmt);
  if (period < 10)
  {
    runtime_error(stmt.line,
                  "el periodo de 'every' es demasiado corto, uso 10 ms");
    period = 10;
  }

  Handler handler;
  handler.stmt = &stmt;
  handler.period_ms = period;
  _every_handlers.push_back(handler);

  StrideLogger::Log(StrideSubsystem::Interpreter,
                    "Handler 'every %u ms' registrado", (unsigned)period);
}

void Interpreter::run_handlers()
{
  if (_when_handlers.empty() && _every_handlers.empty())
    return;
  if (should_stop())
    return;

  StrideLogger::Log(StrideSubsystem::Interpreter,
                    "Entrando en modo reactivo (%u when, %u every)",
                    (unsigned)_when_handlers.size(),
                    (unsigned)_every_handlers.size());

  uint32_t now = now_ms();
  for (auto &handler : _every_handlers)
    handler.next_due = now + handler.period_ms;

  while (!should_stop())
  {
    // One edge sample per button per tick, shared by all its handlers, so
    // two handlers on the same button both see the same press.
    struct ButtonEdges
    {
      StrideButton *button;
      bool pressed;
      bool released;
    };
    std::vector<ButtonEdges> edges;

    for (auto &handler : _when_handlers)
    {
      ButtonEdges *sample = nullptr;
      for (auto &edge : edges)
        if (edge.button == handler.button)
          sample = &edge;

      if (!sample)
      {
        edges.push_back({handler.button, handler.button->just_pressed(),
                         handler.button->just_released()});
        sample = &edges.back();
      }

      bool fired = handler.stmt->flags ? sample->pressed : sample->released;
      if (fired)
        exec_block(handler.stmt->body);

      if (should_stop())
        return;
    }

    now = now_ms();
    for (auto &handler : _every_handlers)
    {
      if ((int32_t)(now - handler.next_due) >= 0)
      {
        handler.next_due += handler.period_ms;
        exec_block(handler.stmt->body);
      }
      if (should_stop())
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
