#include "interpreter.hpp"

static bool is_expr_assignment(const std::vector<Token> &tokens)
{
  return tokens.size() >= 3 &&
         tokens[0].type == TokenType::IDENTIFIER &&
         tokens[1].type == TokenType::ASSIGN;
}

static bool is_arrow_assignment(const std::vector<Token> &tokens)
{
  return tokens.size() == 3 &&
         tokens[1].type == TokenType::ARROW;
}

void Interpreter::execute(const StrideProgram &program)
{
  if (program.empty())
  {
    StrideLogger::Log(StrideSubsystem::Interpreter, "File %s is empty", Blackboard::CurrentProgram.get());
    return;
  }

  for (size_t index = 0; index < program.size(); index++)
  {
    const auto &tokens = program[index];

    if (tokens.empty())
      continue;

    if (is_expr_assignment(tokens))
    {
      execute_expr_allocation(tokens);
      continue;
    }

    if (is_arrow_assignment(tokens))
    {
      execute_arrow_allocation(tokens);
      continue;
    }

    switch (tokens[0].type)
    {
    case TokenType::FILE:
      executeLogfile(tokens);
      break;

    case TokenType::DEVICE:
      load_device_command(tokens);
      break;

    case TokenType::WRITE:
      execute_write_command(tokens);
      break;

    case TokenType::WAIT:
      execute_wait_command(tokens);
      break;

    case TokenType::ARROW:
      execute_arrow_allocation(tokens);
      break;

    case TokenType::PRINT:
      execute_print_command(tokens);
      break;

    case TokenType::I2C:
      execute_I2C_(tokens);
      break;

    case TokenType::SIGN16:
      execute_sign16_command(tokens);
      break;

    case TokenType::LOOP:
      index = execute_control_loop(program, index);
      break;

    case TokenType::DLOOP:
      break;

    case TokenType::IF:
      index = execute_control_if(program, index);
      break;

    default:
    {
      std::stringstream ss;
      ss << tokens[0].type;
      StrideLogger::Warning(StrideSubsystem::Interpreter,
                            "Unknown command. Type=%s, Value='%s'",
                            ss.str().c_str(), tokens[0].value.c_str());
      break;
    }
    }
  }
}

void Interpreter::execute_simple_block_command(const std::vector<Token> &tokens)
{
  if (tokens.empty())
    return;

  if (is_expr_assignment(tokens))
  {
    execute_expr_allocation(tokens);
    return;
  }

  if (is_arrow_assignment(tokens))
  {
    execute_arrow_allocation(tokens);
    return;
  }

  switch (tokens[0].type)
  {
  case TokenType::DEVICE:
    load_device_command(tokens);
    break;

  case TokenType::WRITE:
    execute_write_command(tokens);
    break;

  case TokenType::WAIT:
    execute_wait_command(tokens);
    break;

  case TokenType::PRINT:
    execute_print_command(tokens);
    break;

  case TokenType::I2C:
    execute_I2C_(tokens);
    break;

  case TokenType::SIGN16:
    execute_sign16_command(tokens);
    break;

  case TokenType::ARROW:
    execute_arrow_allocation(tokens);
    break;

  default:
    StrideLogger::Warning(StrideSubsystem::Interpreter,
                          "Unknown command inside block: %s",
                          tokens[0].get_value());
    break;
  }
}

void Interpreter::execute_range(const StrideProgram &program, size_t start, size_t end)
{
  for (size_t i = start; i < end; ++i)
  {
    if (i >= program.size())
      break;

    const auto &line_tokens = program[i];
    if (line_tokens.empty())
      continue;

    TokenType type = line_tokens[0].type;

    if (type == TokenType::IF)
      i = execute_control_if(program, i);
    else if (type == TokenType::LOOP)
      i = execute_control_loop(program, i);
    else if (type == TokenType::DLOOP || type == TokenType::ENDIF)
      continue;
    else
      execute_simple_block_command(line_tokens);
  }
}
