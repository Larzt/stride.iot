#include "interpreter.hpp"

void Interpreter::execute(const StrideProgram &program)
{
  if (program.empty())
  {
    StrideLogger::Log(StrideSubsystem::Interpreter, "File %s is empty", Blackboard::CurrentLoadProgramFile.get());
    return;
  }

  for (size_t index = 0; index < program.size(); index++)
  {
    const auto &tokens = program[index];

    if (tokens.empty())
    {
      continue;
    }

    const bool is_assign = tokens.size() == 3;
    if (is_assign)
    {
      if (tokens[1].type == TokenType::ARROW)
      {
        execute_arrow_allocation(tokens);
        continue;
      }
      else if (tokens[1].type == TokenType::ASSIGN)
      {
        execute_simple_allocation(tokens);
        continue;
      }
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
      StrideLogger::Warning(StrideSubsystem::Interpreter, "Unknown command. Type=%s, Value='%s'", ss.str().c_str(), tokens[0].value.c_str());
      break;
    }
    }
  }
}

/**
 *
 */
void Interpreter::executeLogfile(const std::vector<Token> &tokens)
{
}

/**
 *
 */
void Interpreter::execute_simple_block_command(const std::vector<Token> &tokens)
{
  if (tokens.empty())
    return;

  const bool is_assign = tokens.size() == 3;
  if (is_assign)
  {
    if (tokens[1].type == TokenType::ARROW)
    {
      execute_arrow_allocation(tokens);
      return;
    }
    else if (tokens[1].type == TokenType::ASSIGN)
    {
      execute_simple_allocation(tokens);
      return;
    }
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

  default:
    StrideLogger::Warning(StrideSubsystem::Interpreter, "Unknown command type inside block with value: %s", tokens[0].get_value());
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
    {
      i = execute_control_if(program, i);
    }
    else if (type == TokenType::LOOP)
    {
      i = execute_control_loop(program, i);
    }
    else if (type == TokenType::DLOOP || type == TokenType::ENDIF)
    {
      continue;
    }
    else
    {
      execute_simple_block_command(line_tokens);
    }
  }
}

/**
 *
 */
void Interpreter::load_device_command(const std::vector<Token> &tokens)
{
  std::string name;
  int pin = -1;
  TokenType device_type = TokenType::UNKNOWN;

  for (size_t i = 0; i < tokens.size(); i++)
  {
    if (i + 2 >= tokens.size())
    {
      break;
    }

    if (tokens[i].type == TokenType::DEVICE)
    {
      device_type = tokens[i + 2].type;
    }
    else if (tokens[i].type == TokenType::NAME)
    {
      name = tokens[i + 2].value;
    }
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

  if (!name.empty() && pin >= 0 && device_type != TokenType::UNKNOWN)
  {
    if (!GPIO_IS_VALID_GPIO(pin))
    {
      StrideLogger::Error(StrideSubsystem::Interpreter, "Pin %d is not a valid GPIO", pin);
      return;
    }

    if (device_type == TokenType::LED)
    {
      StrideLogger::Log(StrideSubsystem::Interpreter, "Creating LED %s on pin %d", name.c_str(), pin);
      StrideLed *led = new StrideLed((gpio_num_t)pin);
      _leds[name] = led;
    }
    else if (device_type == TokenType::BUTTON)
    {
      StrideLogger::Log(StrideSubsystem::Interpreter, "Creating BUTTON %s on pin %d", name.c_str(), pin);
      StrideButton *btn = new StrideButton((gpio_num_t)pin);
      _buttons[name] = btn;
    }
    else if (device_type == TokenType::BUZZER)
    {
      StrideLogger::Log(StrideSubsystem::Interpreter, "Creating BUZZER %s on pin %d", name.c_str(), pin);
      StrideBuzzer *bzr = new StrideBuzzer((gpio_num_t)pin);
      _buzzers[name] = bzr;
    }
    else
    {
      StrideLogger::Error(StrideSubsystem::Interpreter, "Unknown device type for '%s'", name.c_str());
    }
  }
  else
  {
    StrideLogger::Error(StrideSubsystem::Interpreter, "Invalid device declaration: name='%s', pin=%d", name.c_str(), pin);
  }
}

int Interpreter::resolve_value(const Token &token)
{
  if (token.type == TokenType::NUMBER)
  {
    return std::stoi(token.value);
  }

  if (token.type == TokenType::VALUE)
  {
    return (token.value == "on" || token.value == "ON") ? 1 : 0;
  }

  if (token.type == TokenType::IDENTIFIER || token.type == TokenType::NAME)
  {
    if (_buttons.find(token.value) != _buttons.end())
    {
      bool state = _buttons[token.value]->is_pressed();
      StrideLogger::Log(StrideSubsystem::Interpreter,
                        "BUTTON %s state = %d",
                        token.value.c_str(), state);
      return state ? 1 : 0;
    }

    if (_leds.find(token.value) != _leds.end())
    {
      return 0;
    }

    if (_variables.find(token.value) != _variables.end())
    {
      return _variables[token.value];
    }

    StrideLogger::Error(StrideSubsystem::Interpreter, "Unknown identifier '%s'", token.value.c_str());
    return 0;
  }

  StrideLogger::Error(StrideSubsystem::Interpreter, "Cannot resolve value for token type: %s", token.type);
  return 0;
}

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

  if (_leds.find(name) != _leds.end())
  {
    StrideLed *led = _leds[name];

    if (value)
    {
      led->on();
    }
    else
    {
      led->off();
    }
    StrideLogger::Log(StrideSubsystem::Interpreter, "Led %s -> %s", name.c_str(), state.c_str());
    return;
  }

  if (_buzzers.find(name) != _buzzers.end())
  {
    StrideBuzzer *buzzer = _buzzers[name];

    if (value)
    {
      buzzer->on();
    }
    else
    {
      buzzer->off();
    }
    StrideLogger::Log(StrideSubsystem::Interpreter, "Buzzer %s -> %s", name.c_str(), state.c_str());
    return;
  }

  StrideLogger::Error(StrideSubsystem::Interpreter, "Device %s not found", name.c_str());
}

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
    {
      message += token.value;
    }
    else if (_variables.find(token.value) != _variables.end())
    {
      message += std::to_string(_variables[token.value]);
    }
    else
    {
      message += token.value;
    }

    if (i + 1 < tokens.size())
    {
      message += " ";
    }
  }

  std::string timestamp_msg = "[HORA]: " + message + "\n";
  std::string path = Blackboard::MountPoint + Blackboard::CurrentLogFile;
  sink_file(path, timestamp_msg);
}

void Interpreter::execute_arrow_allocation(const std::vector<Token> &tokens)
{
  if (tokens.size() < 3)
  {
    std::string found = "";
    for (const auto &token : tokens)
    {
      found += token.value + " ";
    }

    StrideLogger::Error(StrideSubsystem::Interpreter, "Expected <value> -> <var>, found: %s", found.c_str());
    return;
  }

  std::string variable_value_str = "";
  std::string variable_name = "";

  for (size_t i = 0; i < tokens.size(); i++)
  {
    if (tokens[i].type == TokenType::ARROW)
    {
      variable_value_str = tokens[i - 1].value;
      variable_name = tokens[i + 1].value;
    }
  }

  char *end_ptr = nullptr;
  long variable_value = std::strtol(variable_value_str.c_str(), &end_ptr, 10);

  if (end_ptr == variable_value_str.c_str() || *end_ptr != '\0')
  {
    StrideLogger::Error(StrideSubsystem::Interpreter, "Invalid variable value: %s", variable_value_str.c_str());
    return;
  }

  _variables[variable_name] = variable_value;
}

void Interpreter::execute_simple_allocation(const std::vector<Token> &tokens)
{
  if (tokens.size() < 3)
  {
    std::string found = "";
    for (const auto &token : tokens)
    {
      found += token.value + " ";
    }
    StrideLogger::Error(StrideSubsystem::Interpreter, "Expected <var> = <value>, found: %s", found.c_str());
    return;
  }

  std::string variable_name = tokens[0].value;
  std::string variable_value_str = tokens[2].value;

  char *end_ptr = nullptr;
  long variable_value = std::strtol(variable_value_str.c_str(), &end_ptr, 10);

  if (end_ptr == variable_value_str.c_str() || *end_ptr != '\0')
  {
    StrideLogger::Error(StrideSubsystem::Interpreter, "Invalid variable value: %s", variable_value_str.c_str());
    return;
  }

  _variables[variable_name] = variable_value;
}

bool Interpreter::evaluate_condition(const std::vector<Token> &tokens)
{
  if (tokens.size() < 4)
    return false;

  std::string varName = tokens[1].value;
  TokenType op = tokens[2].type;
  std::string targetStr = tokens[3].value;

  // Variable
  if (_variables.find(varName) != _variables.end())
  {
    int varVal = _variables[varName];
    int targetVal = std::stoi(targetStr);

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

  // Output / GPIO
  if (_leds.find(varName) != _leds.end())
  {
    int pinState = _leds[varName]->get();
    int expected = (targetStr == "on") ? 1 : 0;

    if (op == TokenType::IS_EQUAL)
      return pinState == expected;

    if (op == TokenType::NOT_EQUAL)
      return pinState != expected;

    return false;
  }

  // Button / input GPIO
  if (_buttons.find(varName) != _buttons.end())
  {
    int pinState = _buttons[varName]->is_pressed() ? 1 : 0;
    int expected = (targetStr == "on" || targetStr == "1") ? 1 : 0;

    if (op == TokenType::IS_EQUAL)
      return pinState == expected;

    if (op == TokenType::NOT_EQUAL)
      return pinState != expected;

    return false;
  }

  return false;
}

size_t Interpreter::execute_control_loop(const StrideProgram &program, size_t index)
{
  const StrideProgram subprogram;
  const auto &tokens = program[index];

  if (tokens.size() < 2)
  {
    StrideLogger::Error(StrideSubsystem::Interpreter, "Invalid LOOP syntax at line %zu: ", index + 1);
    return index;
  }

  size_t loop_start = index + 1;
  size_t loop_end = loop_start;

  bool has_dloop = false;
  while (loop_end < program.size())
  {
    if (program[loop_end][0].type == TokenType::DLOOP)
    {
      has_dloop = true;
      break;
    }
    loop_end++;
  }

  if (!has_dloop)
  {
    StrideLogger::Error(StrideSubsystem::Interpreter, "Invalid LOOP syntax there is no DLOOP", index + 1);
    return index;
  }

  bool match = (evaluate_condition(tokens) && tokens.size() > 2);

  if (match)
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
    ESP_LOGE("Interpreter IF", "Invalid IF syntax at line %zu. Expected: IF <var> <op> <value>", index + 1);
    return index;
  }

  bool conditionMatch = evaluate_condition(tokens);

  size_t blockStart = index + 1;
  size_t blockEnd = blockStart;
  size_t elseIndex = SIZE_MAX;

  while (blockEnd < program.size())
  {
    TokenType t = program[blockEnd][0].type;
    if (t == TokenType::ELSE)
    {
      elseIndex = blockEnd;
    }
    else if (t == TokenType::ENDIF)
    {
      break;
    }
    blockEnd++;
  }

  if (conditionMatch)
  {
    size_t execEnd = (elseIndex != SIZE_MAX) ? elseIndex : blockEnd;
    for (size_t i = blockStart; i < execEnd; i++)
    {
      execute_simple_block_command(program[i]);
    }
  }
  else if (elseIndex != SIZE_MAX)
  {
    for (size_t i = elseIndex + 1; i < blockEnd; i++)
    {
      execute_simple_block_command(program[i]);
    }
  }

  return blockEnd;
}

//
// I2C Execute command
//

void Interpreter::executeI2C(const std::vector<Token> &tokens)
{
  if (tokens.size() < 2)
  {
    ESP_LOGE("Interpreter I2C", "Incomplete I2C command");
    return;
  }

  switch (tokens[1].type)
  {
  case TokenType::INIT:
    executeI2CInit(tokens);
    break;

  case TokenType::WRITE:
    executeI2CWrite(tokens);
    break;

  case TokenType::READ:
    executeI2CRead(tokens);
    break;

  default:
    ESP_LOGW("Interpreter I2C", "Unknown I2C subcommand");
    break;
  }
}

void Interpreter::executeI2CInit(const std::vector<Token> &tokens)
{
  int sda = -1;
  int scl = -1;

  for (size_t i = 0; i < tokens.size(); i++)
  {
    bool limit = i + 1 < tokens.size();
    if (tokens[i].type == TokenType::SDA && limit)
    {
      sda = std::stoi(tokens[i + 1].value);
    }

    if (tokens[i].type == TokenType::SDA && limit)
    {
      scl = std::stoi(tokens[i + 1].value);
    }
  }

  if (sda == -1 || scl == -1)
  {
    ESP_LOGE("Interpreter I2C", "Invalid INIT syntaxis");
    return;
  }

  ESP_LOGI("Interpreter I2C", "Init I2C SDA=%d SCL=%d", sda, scl);

  // Inicializar el I2C con el driver
  // i2c_master_init((gpio_num_t)sda, (gpio_num_t)scl);
}

void Interpreter::executeI2CWrite(const std::vector<Token> &tokens)
{
  if (tokens.size() < 5)
  {
    ESP_LOGE("Interpreter I2C", "Invalid WRITE syntax");
    return;
  }

  int addr = parse_hex_number(tokens[2].value);
  int reg = parse_hex_number(tokens[3].value);
  int data = parse_hex_number(tokens[4].value);

  ESP_LOGI("Interpreter I2C", "WRITE addr=0x%X reg=0x%X data=0x%X",
           addr, reg, data);

  // i2c_write_byte(addr, reg, data);
}

void Interpreter::executeI2CRead(const std::vector<Token> &tokens)
{
  // Sintaxis mínima: I2C READ addr reg bytes
  // Sintaxis opcional: I2C READ addr reg bytes => var
  if (tokens.size() < 5)
  {
    ESP_LOGE("Interpreter I2C", "Invalid READ syntax");
    return;
  }

  int addr = parse_hex_number(tokens[2].value);
  int reg = parse_hex_number(tokens[3].value);
  int bytes = std::stoi(tokens[4].value);

  ESP_LOGI("Interpreter I2C", "READ addr=0x%X reg=0x%X bytes=%d", addr, reg, bytes);

  int result = 1234; // placeholder: i2c_read_bytes(addr, reg, bytes);

  std::string varName = "";

  for (size_t i = 0; i < tokens.size(); i++)
  {
    if (tokens[i].type == TokenType::ARROW && i + 1 < tokens.size())
    {
      varName = tokens[i + 1].value;
    }
  }

  if (varName != "")
  {
    _variables[varName] = result;
    ESP_LOGI("Interpreter I2C", "Stored %d in %s", result, varName.c_str());
  }
  else
  {
    ESP_LOGI("Interpreter I2C", "Read result (not stored): %d", result);
  }
}
