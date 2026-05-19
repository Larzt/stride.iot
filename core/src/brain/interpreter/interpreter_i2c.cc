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
