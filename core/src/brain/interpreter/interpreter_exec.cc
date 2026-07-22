#include "interpreter.hpp"

#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sink.hpp"
#include "timer.hpp"

void Interpreter::exec_block(const lang::Block &block)
{
  for (const auto &stmt : block)
  {
    if (should_stop())
      return;
    exec_stmt(stmt);
  }
}

void Interpreter::exec_stmt(const lang::Stmt &stmt)
{
  using Kind = lang::Stmt::Kind;

  switch (stmt.kind)
  {
  case Kind::DeclareDevice:
    declare_device(stmt);
    break;

  case Kind::Turn:
    exec_turn(stmt);
    break;

  case Kind::Toggle:
    exec_toggle(stmt);
    break;

  case Kind::Set:
    if (!stmt.args.empty())
      _variables[stmt.name] = eval(stmt.args[0].get());
    break;

  case Kind::Wait:
    exec_wait(stmt);
    break;

  case Kind::Print:
    exec_print(stmt);
    break;

  case Kind::Show:
    exec_show(stmt);
    break;

  case Kind::LogTo:
    exec_log_to(stmt);
    break;

  case Kind::Stop:
    StrideLogger::Log(StrideSubsystem::Interpreter,
                      "STOP en linea %u: fin del programa",
                      (unsigned)stmt.line);
    _stop_requested = true;
    break;

  case Kind::If:
    exec_if(stmt);
    break;

  case Kind::Repeat:
    exec_repeat(stmt);
    break;

  case Kind::When:
    register_when(stmt);
    break;

  case Kind::Every:
    register_every(stmt);
    break;

  case Kind::I2cWrite:
    exec_i2c_write(stmt);
    break;

  case Kind::I2cRead:
    exec_i2c_read(stmt);
    break;
  }
}

void Interpreter::exec_if(const lang::Stmt &stmt)
{
  for (const auto &branch : stmt.branches)
  {
    bool taken = (branch.first == nullptr) || (eval(branch.first.get()) != 0);
    if (taken)
    {
      exec_block(branch.second);
      return;
    }
  }
}

void Interpreter::exec_repeat(const lang::Stmt &stmt)
{
  using Mode = lang::RepeatMode;
  Mode mode = static_cast<Mode>(stmt.flags);

  const lang::Expr *cond =
      stmt.args.empty() ? nullptr : stmt.args[0].get();

  switch (mode)
  {
  case Mode::Times:
  {
    int32_t count = cond ? eval(cond) : 0;
    for (int32_t i = 0; i < count && !should_stop(); i++)
    {
      exec_block(stmt.body);
      vTaskDelay(pdMS_TO_TICKS(10));
    }
    break;
  }

  case Mode::Forever:
    while (!should_stop())
    {
      exec_block(stmt.body);
      vTaskDelay(pdMS_TO_TICKS(10));
    }
    break;

  case Mode::While:
    while (!should_stop() && cond && eval(cond) != 0)
    {
      exec_block(stmt.body);
      vTaskDelay(pdMS_TO_TICKS(10));
    }
    break;

  case Mode::Until:
    while (!should_stop() && cond && eval(cond) == 0)
    {
      exec_block(stmt.body);
      vTaskDelay(pdMS_TO_TICKS(10));
    }
    break;
  }
}

// Durations come either pre-resolved by the parser (flags=0, value=ms) or as
// an expression with the unit factor in value (flags=1).
uint32_t Interpreter::resolve_duration_ms(const lang::Stmt &stmt)
{
  if (stmt.flags == 0 || stmt.args.empty())
    return stmt.value;

  int32_t magnitude = eval(stmt.args[0].get());
  if (magnitude <= 0)
    return 0;

  return (uint32_t)magnitude * stmt.value;
}

// Sleeps in small slices so an external stop (another program launched from
// the web or the buttons) interrupts the wait quickly.
void Interpreter::interruptible_delay(uint32_t ms)
{
  uint32_t remaining = ms;
  while (remaining > 0 && !should_stop())
  {
    uint32_t slice = remaining > 50 ? 50 : remaining;
    vTaskDelay(pdMS_TO_TICKS(slice));
    remaining -= slice;
  }
}

void Interpreter::exec_wait(const lang::Stmt &stmt)
{
  interruptible_delay(resolve_duration_ms(stmt));
}

std::string Interpreter::build_output_message(const lang::Stmt &stmt)
{
  std::string message;

  for (size_t i = 0; i < stmt.args.size(); i++)
  {
    const lang::Expr *arg = stmt.args[i].get();
    if (!arg)
      continue;

    if (arg->kind == lang::Expr::Kind::String)
      message += arg->text;
    else
      message += std::to_string(eval(arg));

    if (i + 1 < stmt.args.size())
      message += " ";
  }

  return message;
}

void Interpreter::exec_print(const lang::Stmt &stmt)
{
  std::string message = build_output_message(stmt);

  StrideLogger::Log(StrideSubsystem::Interpreter, "print: %s",
                    message.c_str());

  std::string entry = "[" + TimeUtils::get_timestamp() + "]: " + message + "\n";
  sink_file(Blackboard::MountPoint + Blackboard::CurrentLogFile, entry);
}

void Interpreter::exec_show(const lang::Stmt &stmt)
{
  std::string message = build_output_message(stmt);

  StrideLogger::Log(StrideSubsystem::Interpreter, "show: %s",
                    message.c_str());

  Blackboard::DslShowText = message;
}

void Interpreter::exec_log_to(const lang::Stmt &stmt)
{
  std::string filename = stmt.name;
  if (filename.empty())
    return;

  if (filename[0] != '/')
    filename = "/" + filename;

  std::string path = Blackboard::MountPoint + filename;
  bool created = (access(path.c_str(), F_OK) != 0);

  FILE *f = fopen(path.c_str(), "a");
  if (f == nullptr)
  {
    runtime_error(stmt.line, "no se pudo crear el archivo '" + stmt.name + "'");
    return;
  }
  fclose(f);

  Blackboard::CurrentLogFile = filename;
  if (created)
    Blackboard::FileListVersion = Blackboard::FileListVersion.get() + 1;

  StrideLogger::Log(StrideSubsystem::Interpreter, "Log file activo: %s",
                    path.c_str());
}
