#include "interpreter.hpp"

// ─────────────────────────────────────────────
//  Dispatcher principal: I2C INIT / WRITE / READ
// ─────────────────────────────────────────────
void Interpreter::executeI2C(const std::vector<Token> &tokens)
{
  // tokens[0] = I2C, tokens[1] = INIT | WRITE | READ
  if (tokens.size() < 2)
  {
    ESP_LOGE("I2C", "Comando I2C incompleto");
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
  case TokenType::READLE:
    executeI2CReadLE(tokens);
    break;
  default:
    ESP_LOGE("I2C", "Subcomando desconocido: %s", tokens[1].value.c_str());
  }
}

// ─────────────────────────────────────────────
//  I2C INIT
//  Sintaxis: I2C INIT
//  Los pines SDA/SCL están fijos en i2c_bus.hpp
// ─────────────────────────────────────────────
void Interpreter::executeI2CInit(const std::vector<Token> &tokens)
{
  if (_i2c_initialized)
  {
    ESP_LOGW("I2C", "Bus ya inicializado, ignorando INIT");
    return;
  }

  // No inicializamos el bus, solo verificamos que ya existe
  if (i2c_get_bus() == nullptr)
  {
    ESP_LOGE("I2C", "Bus I2C no disponible. Asegurate de llamar a i2c_master_init() en el arranque");
    return;
  }

  _i2c_initialized = true;
  ESP_LOGI("I2C", "Bus I2C adquirido correctamente");
}
// ─────────────────────────────────────────────
//  Helper: obtiene o crea el handle para una dirección
// ─────────────────────────────────────────────
i2c_master_dev_handle_t Interpreter::i2c_get_or_create_device(uint8_t addr, uint32_t speed_hz)
{
  auto it = _i2c_devices.find(addr);
  if (it != _i2c_devices.end())
  {
    return it->second; // ya existe
  }

  if (!_i2c_initialized)
  {
    ESP_LOGE("I2C", "Bus no inicializado. Llama a I2C INIT primero");
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
    ESP_LOGE("I2C", "Error añadiendo dispositivo 0x%02X: %s", addr, esp_err_to_name(err));
    return nullptr;
  }

  _i2c_devices[addr] = handle;
  ESP_LOGI("I2C", "Dispositivo 0x%02X registrado", addr);
  return handle;
}

// ─────────────────────────────────────────────
//  I2C WRITE
//  Sintaxis 1: I2C WRITE <addr> <reg> <data>
//  Ejemplo 1:  I2C WRITE 0x27 0x00 0xFE
// ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─
//  Sintaxis 2: I2C WRITE <addr> <data>
//  Ejemplo 1:  I2C WRITE 0x27 0xFE
// ─────────────────────────────────────────────
void Interpreter::executeI2CWrite(const std::vector<Token> &tokens)
{
  std::vector<int> values;

  for (size_t i = 1; i < tokens.size(); i++)
  {
    if (tokens[i].type == TokenType::HEX_NUMBER || tokens[i].type == TokenType::NUMBER)
    {
      values.push_back(parse_hex_number(tokens[i].value));
    }
  }

  // Sintaxis: I2C WRITE <addr> <data>          (PCF8574, sin registro)
  // Sintaxis: I2C WRITE <addr> <reg> <data>    (con registro)
  if (values.size() < 2)
  {
    ESP_LOGE("I2C", "WRITE: sintaxis invalida. Uso: I2C WRITE <addr> <data> [<reg>]");
    return;
  }

  i2c_master_dev_handle_t dev = i2c_get_or_create_device((uint8_t)values[0]);
  if (!dev)
    return;

  esp_err_t err;

  if (values.size() == 2)
  {
    // Sin registro — PCF8574 y similares
    uint8_t buf[1] = {(uint8_t)values[1]};
    err = i2c_master_transmit(dev, buf, 1, pdMS_TO_TICKS(100));
    ESP_LOGI("I2C", "WRITE addr=0x%02X data=0x%02X", values[0], values[1]);
  }
  else
  {
    // Con registro — sensores, memorias, etc.
    uint8_t buf[2] = {(uint8_t)values[1], (uint8_t)values[2]};
    err = i2c_master_transmit(dev, buf, 2, pdMS_TO_TICKS(100));
    ESP_LOGI("I2C", "WRITE addr=0x%02X reg=0x%02X data=0x%02X", values[0], values[1], values[2]);
  }

  if (err != ESP_OK)
  {
    ESP_LOGE("I2C", "WRITE error: %s", esp_err_to_name(err));
  }
}

// ─────────────────────────────────────────────
//  I2C READ
//  Sintaxis: I2C READ <addr> <reg> <bytes> -> <var>
//  Ejemplo:  I2C READ 0x27 0x00 1 -> resultado
// ─────────────────────────────────────────────
//  Sintaxis: I2C READ <addr> <bytes> -> <var>
//  Ejemplo:  I2C READ 0x27 1 -> resultado
// ─────────────────────────────────────────────
void Interpreter::executeI2CRead(const std::vector<Token> &tokens)
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

  // I2C READ <addr> <bytes> -> var          (sin registro, PCF8574)
  // I2C READ <addr> <reg> <bytes> -> var    (con registro, sensores)
  if (values.size() < 2)
  {
    ESP_LOGE("I2C", "READ: sintaxis invalida");
    return;
  }

  uint8_t addr = (uint8_t)values[0];
  uint8_t reg = (values.size() >= 3) ? (uint8_t)values[1] : 0xFF; // 0xFF = sin registro
  int bytes = (values.size() >= 3) ? values[2] : values[1];
  bool has_reg = (values.size() >= 3);
  (void)has_reg;

  if (bytes < 1 || bytes > 32)
  {
    ESP_LOGE("I2C", "READ: bytes invalido (%d)", bytes);
    return;
  }
  ESP_LOGI("I2C READ", "addr=0x%02X reg=0x%02X bytes=%d var='%s'",
           addr, reg, bytes, varName.c_str());

  i2c_master_dev_handle_t dev = i2c_get_or_create_device((uint8_t)addr);
  if (!dev)
  {
    ESP_LOGE("I2C READ", "No se pudo obtener device handle");
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
    ESP_LOGE("I2C READ", "transmit_receive falló: %s", esp_err_to_name(err));
    return;
  }

  ESP_LOGI("I2C READ", "rx_buf[0]=0x%02X rx_buf[1]=0x%02X", rx_buf[0], rx_buf[1]);

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

  ESP_LOGI("I2C READ", "resultado=%d -> guardando en '%s'", result, varName.c_str());

  if (!varName.empty())
  {
    _variables[varName] = result;
  }
}

// ─────────────────────────────────────────────
//  I2C READLE
//  Sintaxis: I2C READLE <addr> <reg> <bytes> -> <var>
//  Lee <bytes> bytes en orden little-endian (LSB primero).
//  Para 2 bytes aplica corrección de signo automática.
// ─────────────────────────────────────────────
void Interpreter::executeI2CReadLE(const std::vector<Token> &tokens)
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
    ESP_LOGE("I2C", "READLE: sintaxis invalida. Uso: I2C READLE <addr> <reg> <bytes> -> <var>");
    return;
  }

  uint8_t addr  = (uint8_t)values[0];
  uint8_t reg   = (uint8_t)values[1];
  int     bytes = values[2];

  if (bytes < 1 || bytes > 4)
  {
    ESP_LOGE("I2C", "READLE: bytes invalido (%d), rango 1-4", bytes);
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
    ESP_LOGE("I2C", "READLE: transmit_receive falló: %s", esp_err_to_name(err));
    return;
  }

  // Little-endian: el primer byte recibido es el LSB
  int result = 0;
  for (int i = bytes - 1; i >= 0; i--)
    result = (result << 8) | rx_buf[i];

  // Corrección de signo automática para valores de 16 bits
  if (bytes == 2 && result > 32767)
    result -= 65536;

  ESP_LOGI("I2C", "READLE addr=0x%02X reg=0x%02X bytes=%d resultado=%d -> '%s'",
           addr, reg, bytes, result, varName.c_str());

  if (!varName.empty())
    _variables[varName] = result;
}
