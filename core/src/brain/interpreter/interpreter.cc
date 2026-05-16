#include "interpreter.hpp"

#include <unistd.h>

// ─────────────────────────────────────────────
//  Helper: detecta si una línea es una asignación con expresión
//  IDENTIFIER ASSIGN <expr...>
// ─────────────────────────────────────────────
static bool is_expr_assignment(const std::vector<Token> &tokens)
{
  return tokens.size() >= 3 &&
         tokens[0].type == TokenType::IDENTIFIER &&
         tokens[1].type == TokenType::ASSIGN;
}

// ─────────────────────────────────────────────
//  Helper: detecta si una línea es una asignación con arrow
//  <valor> -> IDENTIFIER   (exactamente 3 tokens)
// ─────────────────────────────────────────────
static bool is_arrow_assignment(const std::vector<Token> &tokens)
{
  return tokens.size() == 3 &&
         tokens[1].type == TokenType::ARROW;
}

// ─────────────────────────────────────────────
//  execute()
// ─────────────────────────────────────────────
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
      executeI2C(tokens);
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

// ─────────────────────────────────────────────
//  execute_simple_block_command
//  Usado dentro de LOOP, IF, ELSE
// ─────────────────────────────────────────────
void Interpreter::execute_simple_block_command(const std::vector<Token> &tokens)
{
  if (tokens.empty())
    return;

  // Asignación con expresión: var = a + b  (cualquier longitud >= 3)
  if (is_expr_assignment(tokens))
  {
    execute_expr_allocation(tokens);
    return;
  }

  // Asignación con arrow: 0 -> var
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
    executeI2C(tokens);
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

// ─────────────────────────────────────────────
//  execute_range — ejecuta líneas [start, end)
// ─────────────────────────────────────────────
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

// ─────────────────────────────────────────────
//  executeLogfile
//  Sintaxis: FILE "nombre.log"
//  Cambia el fichero de log activo y lo crea si no existe.
// ─────────────────────────────────────────────
void Interpreter::executeLogfile(const std::vector<Token> &tokens)
{
  if (tokens.size() < 2 || tokens[1].type != TokenType::STRING)
  {
    StrideLogger::Error(StrideSubsystem::Interpreter, "FILE: sintaxis invalida. Uso: FILE \"nombre.log\"");
    return;
  }

  std::string filename = tokens[1].value;
  if (filename.empty() || filename[0] != '/')
    filename = "/" + filename;

  std::string path = Blackboard::MountPoint + filename;

  bool created = (access(path.c_str(), F_OK) != 0);

  FILE *f = fopen(path.c_str(), "a");
  if (f == nullptr)
  {
    StrideLogger::Error(StrideSubsystem::Interpreter, "FILE: no se pudo crear '%s'", path.c_str());
    return;
  }
  fclose(f);

  Blackboard::CurrentLogFile = filename;
  if (created)
    Blackboard::FileListVersion = Blackboard::FileListVersion.get() + 1;
  StrideLogger::Log(StrideSubsystem::Interpreter, "Log file activo: %s", path.c_str());
}

// ─────────────────────────────────────────────
//  load_device_command
// ─────────────────────────────────────────────
void Interpreter::load_device_command(const std::vector<Token> &tokens)
{
  std::string name;
  int pin = -1;
  TokenType device_type = TokenType::UNKNOWN;

  for (size_t i = 0; i < tokens.size(); i++)
  {
    if (i + 2 >= tokens.size())
      break;

    if (tokens[i].type == TokenType::DEVICE)
      device_type = tokens[i + 2].type;
    else if (tokens[i].type == TokenType::NAME)
      name = tokens[i + 2].value;
    else if (tokens[i].type == TokenType::PIN)
    {
      std::string val = tokens[i + 2].value;
      val.erase(std::remove_if(val.begin(), val.end(), ::isspace), val.end());
      if (val.empty() || !std::all_of(val.begin(), val.end(), ::isdigit))
      {
        StrideLogger::Error(StrideSubsystem::Interpreter, "'%s' is not a valid pin number", val.c_str());
        return;
      }
      pin = std::stoi(val);
    }
  }

  if (name.empty() || pin < 0 || device_type == TokenType::UNKNOWN)
  {
    StrideLogger::Error(StrideSubsystem::Interpreter,
                        "Invalid device declaration: name='%s', pin=%d", name.c_str(), pin);
    return;
  }

  if (!GPIO_IS_VALID_GPIO(pin))
  {
    StrideLogger::Error(StrideSubsystem::Interpreter, "Pin %d is not a valid GPIO", pin);
    return;
  }

  if (device_type == TokenType::LED)
  {
    StrideLogger::Log(StrideSubsystem::Interpreter, "Creating LED %s on pin %d", name.c_str(), pin);
    _leds[name] = new StrideLed((gpio_num_t)pin);
  }
  else if (device_type == TokenType::BUTTON)
  {
    StrideLogger::Log(StrideSubsystem::Interpreter, "Creating BUTTON %s on pin %d", name.c_str(), pin);
    _buttons[name] = new StrideButton((gpio_num_t)pin);
  }
  else if (device_type == TokenType::BUZZER)
  {
    StrideLogger::Log(StrideSubsystem::Interpreter, "Creating BUZZER %s on pin %d", name.c_str(), pin);
    _buzzers[name] = new StrideBuzzer((gpio_num_t)pin);
  }
  else
  {
    StrideLogger::Error(StrideSubsystem::Interpreter, "Unknown device type for '%s'", name.c_str());
  }
}

// ─────────────────────────────────────────────
//  resolve_value — para WRITE y evaluate_condition
// ─────────────────────────────────────────────
int Interpreter::resolve_value(const Token &token)
{
  if (token.type == TokenType::NUMBER)
    return std::stoi(token.value);

  if (token.type == TokenType::HEX_NUMBER)
    return parse_hex_number(token.value);

  if (token.type == TokenType::VALUE)
    return (token.value == "on" || token.value == "ON") ? 1 : 0;

  if (token.type == TokenType::IDENTIFIER || token.type == TokenType::NAME)
  {
    if (_buttons.count(token.value))
      return _buttons[token.value]->is_pressed() ? 1 : 0;

    if (_leds.count(token.value))
      return _leds[token.value]->get();

    if (_variables.count(token.value))
      return _variables[token.value];

    StrideLogger::Error(StrideSubsystem::Interpreter, "Unknown identifier '%s'", token.value.c_str());
    return 0;
  }

  StrideLogger::Error(StrideSubsystem::Interpreter, "Cannot resolve token: %s", token.get_type());
  return 0;
}

// ─────────────────────────────────────────────
//  execute_write_command
// ─────────────────────────────────────────────
void Interpreter::execute_write_command(const std::vector<Token> &tokens)
{
  if (tokens.size() < 4)
  {
    StrideLogger::Error(StrideSubsystem::Interpreter, "Invalid write syntax");
    return;
  }

  std::string name = tokens[2].value;
  std::string state = tokens[3].value;
  bool value = (state == "on");

  if (_leds.count(name))
  {
    value ? _leds[name]->on() : _leds[name]->off();
    StrideLogger::Log(StrideSubsystem::Interpreter, "Led %s -> %s", name.c_str(), state.c_str());
    return;
  }

  if (_buzzers.count(name))
  {
    value ? _buzzers[name]->on() : _buzzers[name]->off();
    StrideLogger::Log(StrideSubsystem::Interpreter, "Buzzer %s -> %s", name.c_str(), state.c_str());
    return;
  }

  StrideLogger::Error(StrideSubsystem::Interpreter, "Device '%s' not found", name.c_str());
}

// ─────────────────────────────────────────────
//  execute_wait_command
// ─────────────────────────────────────────────
void Interpreter::execute_wait_command(const std::vector<Token> &tokens)
{
  if (tokens.size() < 2)
  {
    StrideLogger::Error(StrideSubsystem::Interpreter, "Invalid WAIT syntax");
    return;
  }

  float seconds = std::stof(tokens[1].value);
  vTaskDelay(pdMS_TO_TICKS(seconds * 1000));
}

// ─────────────────────────────────────────────
//  execute_print_command
// ─────────────────────────────────────────────
void Interpreter::execute_print_command(const std::vector<Token> &tokens)
{
  if (tokens.size() < 2)
  {
    StrideLogger::Error(StrideSubsystem::Interpreter, "Invalid PRINT syntax");
    return;
  }

  std::string message = "";
  for (size_t i = 1; i < tokens.size(); i++)
  {
    const Token &token = tokens[i];

    if (token.type == TokenType::STRING)
      message += token.value;
    else if (_variables.count(token.value))
      message += std::to_string(_variables[token.value]);
    else
      message += token.value;

    if (i + 1 < tokens.size())
      message += " ";
  }

  StrideLogger::Log(StrideSubsystem::Interpreter, "message: %s", message.c_str());

  std::string timestamp = TimeUtils::get_timestamp();
  std::string timestamp_msg = "[" + timestamp + "]: " + message + "\n";
  std::string path = Blackboard::MountPoint + Blackboard::CurrentLogFile;
  sink_file(path, timestamp_msg);
}

// ─────────────────────────────────────────────
//  execute_arrow_allocation
//  Sintaxis: <literal> -> <var>
//  Acepta NUMBER y HEX_NUMBER
// ─────────────────────────────────────────────
void Interpreter::execute_arrow_allocation(const std::vector<Token> &tokens)
{
  if (tokens.size() < 3)
  {
    StrideLogger::Error(StrideSubsystem::Interpreter, "Expected <value> -> <var>");
    return;
  }

  std::string variable_name = "";
  int variable_value = 0;

  for (size_t i = 0; i < tokens.size(); i++)
  {
    if (tokens[i].type == TokenType::ARROW && i + 1 < tokens.size())
    {
      variable_name = tokens[i + 1].value;

      // El valor está justo antes del ARROW
      const Token &val_token = tokens[i - 1];
      if (val_token.type == TokenType::NUMBER)
        variable_value = std::stoi(val_token.value);
      else if (val_token.type == TokenType::HEX_NUMBER)
        variable_value = parse_hex_number(val_token.value);
      else
      {
        StrideLogger::Error(StrideSubsystem::Interpreter,
                            "Invalid value before ->: '%s'", val_token.value.c_str());
        return;
      }
    }
  }

  if (variable_name.empty())
  {
    StrideLogger::Error(StrideSubsystem::Interpreter, "Missing variable name after ->");
    return;
  }

  _variables[variable_name] = variable_value;
}

// ─────────────────────────────────────────────
//  execute_simple_allocation
//  Sintaxis: <var> = <literal>  (exactamente 3 tokens, sin operadores)
// ─────────────────────────────────────────────
void Interpreter::execute_simple_allocation(const std::vector<Token> &tokens)
{
  if (tokens.size() < 3)
  {
    StrideLogger::Error(StrideSubsystem::Interpreter, "Expected <var> = <value>");
    return;
  }

  std::string variable_name = tokens[0].value;
  int variable_value = resolve_expr_token(tokens[2]);
  _variables[variable_name] = variable_value;
}

// ─────────────────────────────────────────────
//  resolve_expr_token
//  Resuelve un token individual como valor numérico
// ─────────────────────────────────────────────
int Interpreter::resolve_expr_token(const Token &token)
{
  if (token.type == TokenType::NUMBER)
    return std::stoi(token.value);

  if (token.type == TokenType::HEX_NUMBER)
    return parse_hex_number(token.value);

  if (token.type == TokenType::IDENTIFIER)
  {
    auto it = _variables.find(token.value);
    if (it != _variables.end())
      return it->second;

    ESP_LOGW("EXPR", "Variable '%s' no encontrada, usando 0", token.value.c_str());
    return 0;
  }

  return 0;
}

// ─────────────────────────────────────────────
//  Helpers para el evaluador recursivo
// ─────────────────────────────────────────────
static bool is_binary_op(TokenType t)
{
  switch (t)
  {
  case TokenType::ADD: case TokenType::SUB:
  case TokenType::MUL: case TokenType::DIV: case TokenType::MOD:
  case TokenType::SHL: case TokenType::SHR:
  case TokenType::BIT_AND: case TokenType::BIT_OR:
    return true;
  default:
    return false;
  }
}

static int apply_op(int lhs, TokenType op, int rhs)
{
  switch (op)
  {
  case TokenType::ADD:     return lhs + rhs;
  case TokenType::SUB:     return lhs - rhs;
  case TokenType::MUL:     return lhs * rhs;
  case TokenType::DIV:     return rhs != 0 ? lhs / rhs : 0;
  case TokenType::MOD:     return rhs != 0 ? lhs % rhs : 0;
  case TokenType::SHL:     return lhs << rhs;
  case TokenType::SHR:     return lhs >> rhs;
  case TokenType::BIT_AND: return lhs & rhs;
  case TokenType::BIT_OR:  return lhs | rhs;
  default:                 return lhs;
  }
}

// ─────────────────────────────────────────────
//  eval_primary — resuelve un valor o sub-expresión entre paréntesis
// ─────────────────────────────────────────────
int Interpreter::eval_primary(const std::vector<Token> &tokens, size_t &pos)
{
  if (pos >= tokens.size())
    return 0;

  if (tokens[pos].type == TokenType::LPAREN)
  {
    pos++; // consume (
    int result = eval_expr(tokens, pos);
    if (pos < tokens.size() && tokens[pos].type == TokenType::RPAREN)
      pos++; // consume )
    return result;
  }

  return resolve_expr_token(tokens[pos++]);
}

// ─────────────────────────────────────────────
//  eval_expr — evalúa una expresión left-to-right con soporte de paréntesis
// ─────────────────────────────────────────────
int Interpreter::eval_expr(const std::vector<Token> &tokens, size_t &pos)
{
  int result = eval_primary(tokens, pos);

  while (pos < tokens.size() && is_binary_op(tokens[pos].type))
  {
    TokenType op = tokens[pos++].type;
    int rhs = eval_primary(tokens, pos);
    result = apply_op(result, op, rhs);
  }

  return result;
}

// ─────────────────────────────────────────────
//  execute_expr_allocation
//  Sintaxis: var = <expr>   con soporte de paréntesis
// ─────────────────────────────────────────────
void Interpreter::execute_expr_allocation(const std::vector<Token> &tokens)
{
  if (tokens.size() < 3)
    return;

  std::string var_name = tokens[0].value;
  size_t pos = 2; // salta IDENTIFIER y ASSIGN
  int result = eval_expr(tokens, pos);

  _variables[var_name] = result;
  ESP_LOGI("EXPR", "%s = %d", var_name.c_str(), result);
}

// ─────────────────────────────────────────────
//  evaluate_condition
// ─────────────────────────────────────────────
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

// ─────────────────────────────────────────────
//  execute_sign16_command
//  Sintaxis: SIGN16 <var>
//  Convierte el valor de <var> a entero con signo de 16 bits.
// ─────────────────────────────────────────────
void Interpreter::execute_sign16_command(const std::vector<Token> &tokens)
{
  if (tokens.size() < 2 || tokens[1].type != TokenType::IDENTIFIER)
  {
    StrideLogger::Error(StrideSubsystem::Interpreter, "SIGN16: sintaxis invalida. Uso: SIGN16 <var>");
    return;
  }

  const std::string &name = tokens[1].value;
  auto it = _variables.find(name);
  if (it == _variables.end())
  {
    StrideLogger::Warning(StrideSubsystem::Interpreter, "SIGN16: variable '%s' no encontrada", name.c_str());
    return;
  }

  if (it->second > 32767)
    it->second -= 65536;

  ESP_LOGI("SIGN16", "%s = %d", name.c_str(), it->second);
}

// ─────────────────────────────────────────────
//  execute_control_loop
// ─────────────────────────────────────────────
size_t Interpreter::execute_control_loop(const StrideProgram &program, size_t index)
{
  const auto &tokens = program[index];

  if (tokens.size() < 2)
  {
    StrideLogger::Error(StrideSubsystem::Interpreter, "Invalid LOOP syntax at line %zu", index + 1);
    return index;
  }

  // Localizar el DLOOP correspondiente
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

  // ¿Es un loop condicional? LOOP <var> <op> <val>
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

// ─────────────────────────────────────────────
//  execute_control_if
// ─────────────────────────────────────────────
size_t Interpreter::execute_control_if(const StrideProgram &program, size_t index)
{
  const auto &tokens = program[index];
  if (tokens.size() < 4)
  {
    ESP_LOGE("IF", "Sintaxis invalida en línea %zu. Esperado: IF <var> <op> <val>", index + 1);
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
