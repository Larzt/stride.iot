#include "interpreter.hpp"

#include <unistd.h>

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
