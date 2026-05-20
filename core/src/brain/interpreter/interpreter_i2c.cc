#include "interpreter.hpp"

void Interpreter::execute_I2C_(const std::vector<Token> &tokens)
{

  if (tokens.size() < 2)
  {
    StrideLogger::Error(StrideSubsystem::Interpreter, "I2C: Comando I2C incompleto");
    return;
  }

  switch (tokens[1].type)
  {
  case TokenType::INIT:
    execute_I2C_init(tokens);
    break;
  case TokenType::WRITE:
    execute_I2C_write(tokens);
    break;
  case TokenType::READ:
    execute_I2C_read(tokens);
    break;
  case TokenType::READLE:
    execute_I2C_readLE(tokens);
    break;
  default:
    StrideLogger::Error(StrideSubsystem::Interpreter, "I2C: Subcomando desconocido: %s", tokens[1].value.c_str());
  }
}

void Interpreter::execute_I2C_init(const std::vector<Token> &tokens)
{
  if (_i2c_initialized)
  {
    StrideLogger::Warning(StrideSubsystem::Interpreter, "I2C: Bus ya inicializado, ignorando INIT");
    return;
  }

  if (i2c_get_bus() == nullptr)
  {
    StrideLogger::Error(StrideSubsystem::Interpreter, "I2C: Bus I2C no disponible. Asegurate de llamar a i2c_master_init() en el arranque");
    return;
  }

  _i2c_initialized = true;
  StrideLogger::Log(StrideSubsystem::Interpreter, "I2C: Bus I2C adquirido correctamente");
}

i2c_master_dev_handle_t Interpreter::i2c_get_or_create_device(uint8_t addr, uint32_t speed_hz)
{
  auto it = _i2c_devices.find(addr);
  if (it != _i2c_devices.end())
  {
    return it->second;
  }

  if (!_i2c_initialized)
  {
    StrideLogger::Error(StrideSubsystem::Interpreter, "I2C: Bus no inicializado. Llama a I2C INIT primero");
    return nullptr;
  }

  i2c_device_config_t cfg = {};
  cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
  cfg.device_address = addr;
  cfg.scl_speed_hz = speed_hz;

  i2c_master_dev_handle_t handle = nullptr;
  esp_err_t err = i2c_master_bus_add_device(i2c_get_bus(), &cfg, &handle);

  if (err != ESP_OK)
  {
    StrideLogger::Error(StrideSubsystem::Interpreter, "I2C: Error añadiendo dispositivo 0x%02X: %s", addr, esp_err_to_name(err));
    return nullptr;
  }

  _i2c_devices[addr] = handle;
  StrideLogger::Log(StrideSubsystem::Interpreter, "I2C: Dispositivo 0x%02X registrado", addr);
  return handle;
}

void Interpreter::execute_I2C_write(const std::vector<Token> &tokens)
{
  for (size_t i = 1; i < tokens.size(); i++)
  {
    if (tokens[i].type == TokenType::PIN)
    {
      execute_expander_pin_write(tokens);
      return;
    }
  }

  std::vector<int> values;

  for (size_t i = 1; i < tokens.size(); i++)
  {
    if (tokens[i].type == TokenType::HEX_NUMBER || tokens[i].type == TokenType::NUMBER)
    {
      values.push_back(parse_hex_number(tokens[i].value));
    }
  }

  if (values.size() < 2)
  {
    StrideLogger::Error(StrideSubsystem::Interpreter, "I2C: WRITE: sintaxis invalida. Uso: I2C WRITE <addr> <data> [<reg>]");
    return;
  }

  i2c_master_dev_handle_t dev = i2c_get_or_create_device((uint8_t)values[0]);
  if (!dev)
    return;

  esp_err_t err;

  if (values.size() == 2)
  {

    uint8_t buf[1] = {(uint8_t)values[1]};
    err = i2c_master_transmit(dev, buf, 1, pdMS_TO_TICKS(100));
    StrideLogger::Log(StrideSubsystem::Interpreter, "I2C: WRITE addr=0x%02X data=0x%02X", values[0], values[1]);
  }
  else
  {

    uint8_t buf[2] = {(uint8_t)values[1], (uint8_t)values[2]};
    err = i2c_master_transmit(dev, buf, 2, pdMS_TO_TICKS(100));
    StrideLogger::Log(StrideSubsystem::Interpreter, "I2C: WRITE addr=0x%02X reg=0x%02X data=0x%02X", values[0], values[1], values[2]);
  }

  if (err != ESP_OK)
  {
    StrideLogger::Error(StrideSubsystem::Interpreter, "I2C: WRITE error: %s", esp_err_to_name(err));
  }
}

void Interpreter::execute_I2C_read(const std::vector<Token> &tokens)
{
  for (size_t i = 1; i < tokens.size(); i++)
  {
    if (tokens[i].type == TokenType::PIN)
    {
      execute_expander_pin_read(tokens);
      return;
    }
  }

  std::vector<int> values;
  std::string varName = "";

  for (size_t i = 1; i < tokens.size(); i++)
  {
    if (tokens[i].type == TokenType::ARROW && i + 1 < tokens.size())
    {
      varName = tokens[i + 1].value;
      continue;
    }
    if (tokens[i].type == TokenType::HEX_NUMBER || tokens[i].type == TokenType::NUMBER)
    {
      values.push_back(parse_hex_number(tokens[i].value));
    }
  }

  if (values.size() < 2)
  {
    StrideLogger::Error(StrideSubsystem::Interpreter, "I2C: READ: sintaxis invalida");
    return;
  }

  uint8_t addr = (uint8_t)values[0];
  uint8_t reg = (values.size() >= 3) ? (uint8_t)values[1] : 0xFF;
  int bytes = (values.size() >= 3) ? values[2] : values[1];
  bool has_reg = (values.size() >= 3);
  (void)has_reg;

  if (bytes < 1 || bytes > 32)
  {
    StrideLogger::Error(StrideSubsystem::Interpreter, "I2C: READ: bytes invalido (%d)", bytes);
    return;
  }
  StrideLogger::Log(StrideSubsystem::Interpreter, "I2C READ: addr=0x%02X reg=0x%02X bytes=%d var='%s'",
           addr, reg, bytes, varName.c_str());

  i2c_master_dev_handle_t dev = i2c_get_or_create_device((uint8_t)addr);
  if (!dev)
  {
    StrideLogger::Error(StrideSubsystem::Interpreter, "I2C READ: No se pudo obtener device handle");
    return;
  }

  uint8_t reg_buf = (uint8_t)reg;
  uint8_t rx_buf[32] = {};

  esp_err_t err = i2c_master_transmit_receive(
      dev,
      &reg_buf, 1,
      rx_buf, (size_t)bytes,
      pdMS_TO_TICKS(100)
  );

  if (err != ESP_OK)
  {
    StrideLogger::Error(StrideSubsystem::Interpreter, "I2C READ: transmit_receive falló: %s", esp_err_to_name(err));
    return;
  }

  StrideLogger::Log(StrideSubsystem::Interpreter, "I2C READ: rx_buf[0]=0x%02X rx_buf[1]=0x%02X", rx_buf[0], rx_buf[1]);

  int result = 0;
  int combine = bytes > 4 ? 4 : bytes;
  for (int i = 0; i < combine; i++)
  {
    result = (result << 8) | rx_buf[i];
  }

  if (bytes == 2 && result > 32767)
  {
    result -= 65536;
  }

  StrideLogger::Log(StrideSubsystem::Interpreter, "I2C READ: resultado=%d -> guardando en '%s'", result, varName.c_str());

  if (!varName.empty())
  {
    _variables[varName] = result;
  }
}

void Interpreter::execute_I2C_readLE(const std::vector<Token> &tokens)
{
  std::vector<int> values;
  std::string varName = "";

  for (size_t i = 1; i < tokens.size(); i++)
  {
    if (tokens[i].type == TokenType::ARROW && i + 1 < tokens.size())
    {
      varName = tokens[i + 1].value;
      continue;
    }
    if (tokens[i].type == TokenType::HEX_NUMBER || tokens[i].type == TokenType::NUMBER)
      values.push_back(parse_hex_number(tokens[i].value));
  }

  if (values.size() < 3)
  {
    StrideLogger::Error(StrideSubsystem::Interpreter, "I2C: READLE: sintaxis invalida. Uso: I2C READLE <addr> <reg> <bytes> -> <var>");
    return;
  }

  uint8_t addr = (uint8_t)values[0];
  uint8_t reg = (uint8_t)values[1];
  int bytes = values[2];

  if (bytes < 1 || bytes > 4)
  {
    StrideLogger::Error(StrideSubsystem::Interpreter, "I2C: READLE: bytes invalido (%d), rango 1-4", bytes);
    return;
  }

  i2c_master_dev_handle_t dev = i2c_get_or_create_device(addr);
  if (!dev)
    return;

  uint8_t reg_buf = reg;
  uint8_t rx_buf[4] = {};

  esp_err_t err = i2c_master_transmit_receive(dev, &reg_buf, 1, rx_buf, (size_t)bytes, pdMS_TO_TICKS(100));
  if (err != ESP_OK)
  {
    StrideLogger::Error(StrideSubsystem::Interpreter, "I2C: READLE: transmit_receive falló: %s", esp_err_to_name(err));
    return;
  }

  int result = 0;
  for (int i = bytes - 1; i >= 0; i--)
    result = (result << 8) | rx_buf[i];

  if (bytes == 2 && result > 32767)
    result -= 65536;

  StrideLogger::Log(StrideSubsystem::Interpreter, "I2C: READLE addr=0x%02X reg=0x%02X bytes=%d resultado=%d -> '%s'",
           addr, reg, bytes, result, varName.c_str());

  if (!varName.empty())
    _variables[varName] = result;
}

void Interpreter::execute_expin_declaration(const std::vector<Token> &tokens)
{
  if (tokens.size() < 4 ||
      tokens[1].type != TokenType::IDENTIFIER ||
      tokens[2].type != TokenType::ASSIGN ||
      (tokens[3].type != TokenType::NUMBER && tokens[3].type != TokenType::HEX_NUMBER))
  {
    StrideLogger::Error(StrideSubsystem::Interpreter,
                        "EXPIN: sintaxis invalida. Uso: EXPIN <nombre> = <pin 0-7>");
    return;
  }

  int pin = parse_hex_number(tokens[3].value);
  if (pin < 0 || pin > 7)
  {
    StrideLogger::Error(StrideSubsystem::Interpreter,
                        "EXPIN: pin %d fuera de rango (0-7)", pin);
    return;
  }

  const std::string &name = tokens[1].value;
  _expander_pins[name] = (uint8_t)pin;
  StrideLogger::Log(StrideSubsystem::Interpreter,
                    "EXPIN: alias '%s' -> pin %d del expansor", name.c_str(), pin);
}

bool Interpreter::resolve_expander_pin(const Token &token, uint8_t &pin_out)
{
  if (token.type == TokenType::NUMBER || token.type == TokenType::HEX_NUMBER)
  {
    int p = parse_hex_number(token.value);
    if (p < 0 || p > 7)
      return false;
    pin_out = (uint8_t)p;
    return true;
  }

  if (token.type == TokenType::IDENTIFIER || token.type == TokenType::NAME)
  {
    auto it = _expander_pins.find(token.value);
    if (it != _expander_pins.end())
    {
      pin_out = it->second;
      return true;
    }
  }

  return false;
}

void Interpreter::execute_expander_pin_write(const std::vector<Token> &tokens)
{
  uint8_t pin = 0;
  bool pin_found = false;
  int state = -1;

  for (size_t i = 1; i < tokens.size(); i++)
  {
    if (tokens[i].type == TokenType::PIN &&
        i + 2 < tokens.size() &&
        tokens[i + 1].type == TokenType::ASSIGN)
    {
      if (!resolve_expander_pin(tokens[i + 2], pin))
      {
        StrideLogger::Error(StrideSubsystem::Interpreter,
                            "I2C WRITE PIN: '%s' no es un alias EXPIN ni un pin 0-7",
                            tokens[i + 2].value.c_str());
        return;
      }
      pin_found = true;
      i += 2;
      continue;
    }

    if (tokens[i].type == TokenType::VALUE)
    {
      std::string v = tokens[i].value;
      std::transform(v.begin(), v.end(), v.begin(), ::tolower);
      state = (v == "high" || v == "on" || v == "1") ? 1 : 0;
    }
    else if (tokens[i].type == TokenType::NUMBER)
    {
      state = std::stoi(tokens[i].value) ? 1 : 0;
    }
    else if (tokens[i].type == TokenType::IDENTIFIER && _variables.count(tokens[i].value))
    {
      state = _variables[tokens[i].value] ? 1 : 0;
    }
  }

  if (!pin_found || state < 0)
  {
    StrideLogger::Error(StrideSubsystem::Interpreter,
                        "I2C WRITE PIN: sintaxis invalida. Uso: I2C WRITE PIN=<alias|0-7> <HIGH|LOW>");
    return;
  }

  esp_err_t err = expander_pin_write(pin, state == 1);
  if (err != ESP_OK)
  {
    StrideLogger::Error(StrideSubsystem::Interpreter,
                        "I2C WRITE PIN: error escribiendo pin %d: %s", pin, esp_err_to_name(err));
    return;
  }

  StrideLogger::Log(StrideSubsystem::Interpreter,
                    "I2C WRITE PIN=%d -> %s", pin, state ? "HIGH" : "LOW");
}

void Interpreter::execute_expander_pin_read(const std::vector<Token> &tokens)
{
  uint8_t pin = 0;
  bool pin_found = false;
  std::string varName = "";

  for (size_t i = 1; i < tokens.size(); i++)
  {
    if (tokens[i].type == TokenType::PIN &&
        i + 2 < tokens.size() &&
        tokens[i + 1].type == TokenType::ASSIGN)
    {
      if (!resolve_expander_pin(tokens[i + 2], pin))
      {
        StrideLogger::Error(StrideSubsystem::Interpreter,
                            "I2C READ PIN: '%s' no es un alias EXPIN ni un pin 0-7",
                            tokens[i + 2].value.c_str());
        return;
      }
      pin_found = true;
      i += 2;
      continue;
    }

    if (tokens[i].type == TokenType::ARROW && i + 1 < tokens.size())
    {
      varName = tokens[i + 1].value;
    }
  }

  if (!pin_found)
  {
    StrideLogger::Error(StrideSubsystem::Interpreter,
                        "I2C READ PIN: sintaxis invalida. Uso: I2C READ PIN=<alias|0-7> [-> <var>]");
    return;
  }

  bool level = false;
  esp_err_t err = expander_pin_read(pin, level);
  if (err != ESP_OK)
  {
    StrideLogger::Error(StrideSubsystem::Interpreter,
                        "I2C READ PIN: error leyendo pin %d: %s", pin, esp_err_to_name(err));
    return;
  }

  int result = level ? 1 : 0;
  StrideLogger::Log(StrideSubsystem::Interpreter,
                    "I2C READ PIN=%d -> %d (var='%s')", pin, result, varName.c_str());

  if (!varName.empty())
    _variables[varName] = result;
}
