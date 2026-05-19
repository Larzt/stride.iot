#include "interpreter.hpp"

bool Interpreter::evaluate_condition(const std::vector<Token> &tokens)
{
  if (tokens.size() < 4)
    return false;

  std::string varName = tokens[1].value;
  TokenType op = tokens[2].type;
  int targetVal = resolve_expr_token(tokens[3]);

  int varVal = 0;

  if (_variables.count(varName))
    varVal = _variables[varName];
  else if (_leds.count(varName))
    varVal = _leds[varName]->get();
  else if (_buttons.count(varName))
    varVal = _buttons[varName]->is_pressed() ? 1 : 0;
  else
    return false;

  switch (op)
  {
  case TokenType::IS_EQUAL:
    return varVal == targetVal;
  case TokenType::NOT_EQUAL:
    return varVal != targetVal;
  case TokenType::LESS_THAN:
    return varVal < targetVal;
  case TokenType::LESS_EQUAL:
    return varVal <= targetVal;
  case TokenType::GREATER_THAN:
    return varVal > targetVal;
  case TokenType::GREATER_EQUAL:
    return varVal >= targetVal;
  default:
    return false;
  }
}

size_t Interpreter::execute_control_loop(const StrideProgram &program, size_t index)
{
  const auto &tokens = program[index];

  if (tokens.size() < 2)
  {
    StrideLogger::Error(StrideSubsystem::Interpreter, "Invalid LOOP syntax at line %zu", index + 1);
    return index;
  }

  size_t loop_start = index + 1;
  size_t loop_end = loop_start;
  bool has_dloop = false;

  while (loop_end < program.size())
  {
    if (!program[loop_end].empty() && program[loop_end][0].type == TokenType::DLOOP)
    {
      has_dloop = true;
      break;
    }
    loop_end++;
  }

  if (!has_dloop)
  {
    StrideLogger::Error(StrideSubsystem::Interpreter, "LOOP sin DLOOP en línea %zu", index + 1);
    return index;
  }

  bool is_conditional = (tokens.size() > 2) && evaluate_condition(tokens);

  if (is_conditional)
  {
    while (evaluate_condition(tokens))
    {
      execute_range(program, loop_start, loop_end);
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }
  else
  {
    int repeat_count = std::stoi(tokens[1].value);

    if (repeat_count == -1)
    {
      start_endless_loop();
      while (endless_loop.load())
      {
        execute_range(program, loop_start, loop_end);
        vTaskDelay(pdMS_TO_TICKS(10));
      }
    }
    else
    {
      for (int i = 0; i < repeat_count; ++i)
      {
        execute_range(program, loop_start, loop_end);
        vTaskDelay(pdMS_TO_TICKS(10));
      }
    }
  }

  return loop_end;
}

size_t Interpreter::execute_control_if(const StrideProgram &program, size_t index)
{
  const auto &tokens = program[index];
  if (tokens.size() < 4)
  {
    StrideLogger::Error(StrideSubsystem::Interpreter, "IF: Sintaxis invalida en línea %zu. Esperado: IF <var> <op> <val>", index + 1);
    return index;
  }

  bool condition = evaluate_condition(tokens);

  size_t blockStart = index + 1;
  size_t blockEnd = blockStart;
  size_t elseIndex = SIZE_MAX;

  while (blockEnd < program.size())
  {
    if (program[blockEnd].empty())
    {
      blockEnd++;
      continue;
    }
    TokenType t = program[blockEnd][0].type;
    if (t == TokenType::ELSE)
      elseIndex = blockEnd;
    else if (t == TokenType::ENDIF)
      break;
    blockEnd++;
  }

  if (condition)
  {
    size_t execEnd = (elseIndex != SIZE_MAX) ? elseIndex : blockEnd;
    for (size_t i = blockStart; i < execEnd; i++)
      execute_simple_block_command(program[i]);
  }
  else if (elseIndex != SIZE_MAX)
  {
    for (size_t i = elseIndex + 1; i < blockEnd; i++)
      execute_simple_block_command(program[i]);
  }

  return blockEnd;
}
