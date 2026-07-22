#include "interpreter.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// The bus auto-initializes on the first i2c statement (the old explicit
// `i2c init` no longer exists in the language).
bool Interpreter::i2c_ensure_initialized(uint16_t line)
{
  if (_i2c_initialized)
    return true;

  if (i2c_get_bus() == nullptr)
  {
    runtime_error(line, "el bus I2C no esta disponible en este dispositivo");
    return false;
  }

  _i2c_initialized = true;
  StrideLogger::Log(StrideSubsystem::Interpreter, "I2C: bus adquirido");
  return true;
}

i2c_master_dev_handle_t Interpreter::i2c_get_or_create_device(uint8_t addr,
                                                              uint32_t speed_hz)
{
  auto it = _i2c_devices.find(addr);
  if (it != _i2c_devices.end())
    return it->second;

  i2c_device_config_t cfg = {};
  cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
  cfg.device_address = addr;
  cfg.scl_speed_hz = speed_hz;

  i2c_master_dev_handle_t handle = nullptr;
  esp_err_t err = i2c_master_bus_add_device(i2c_get_bus(), &cfg, &handle);

  if (err != ESP_OK)
  {
    StrideLogger::Error(StrideSubsystem::Interpreter,
                        "I2C: error registrando dispositivo 0x%02X: %s", addr,
                        esp_err_to_name(err));
    return nullptr;
  }

  _i2c_devices[addr] = handle;
  StrideLogger::Log(StrideSubsystem::Interpreter,
                    "I2C: dispositivo 0x%02X registrado", addr);
  return handle;
}

// args = [addr, (reg), value]; flags bit1 = has register.
void Interpreter::exec_i2c_write(const lang::Stmt &stmt)
{
  if (!i2c_ensure_initialized(stmt.line))
    return;

  bool has_register = (stmt.flags & 2) != 0;
  size_t expected_args = has_register ? 3 : 2;
  if (stmt.args.size() < expected_args)
    return; // parse errors already reported

  int32_t addr = eval(stmt.args[0].get());
  if (addr < 0 || addr > 0x7F)
  {
    runtime_error(stmt.line, "direccion I2C fuera de rango (0x00-0x7F)");
    return;
  }

  i2c_master_dev_handle_t dev = i2c_get_or_create_device((uint8_t)addr);
  if (!dev)
    return;

  esp_err_t err;

  if (has_register)
  {
    uint8_t reg = (uint8_t)eval(stmt.args[1].get());
    uint8_t data = (uint8_t)eval(stmt.args[2].get());
    uint8_t buf[2] = {reg, data};
    err = i2c_master_transmit(dev, buf, 2, pdMS_TO_TICKS(100));
    StrideLogger::Log(StrideSubsystem::Interpreter,
                      "I2C write addr=0x%02X reg=0x%02X data=0x%02X",
                      (unsigned)addr, reg, data);
  }
  else
  {
    uint8_t data = (uint8_t)eval(stmt.args[1].get());
    uint8_t buf[1] = {data};
    err = i2c_master_transmit(dev, buf, 1, pdMS_TO_TICKS(100));
    StrideLogger::Log(StrideSubsystem::Interpreter,
                      "I2C write addr=0x%02X data=0x%02X", (unsigned)addr,
                      data);
  }

  if (err != ESP_OK)
    runtime_error(stmt.line, "error escribiendo por I2C: " +
                                 std::string(esp_err_to_name(err)));
}

// args = [addr, (reg), size]; name = destination variable;
// flags bit0 = little endian, bit1 = has register.
void Interpreter::exec_i2c_read(const lang::Stmt &stmt)
{
  if (!i2c_ensure_initialized(stmt.line))
    return;

  bool has_register = (stmt.flags & 2) != 0;
  bool little_endian = (stmt.flags & 1) != 0;
  size_t expected_args = has_register ? 3 : 2;
  if (stmt.args.size() < expected_args || stmt.name.empty())
    return; // parse errors already reported

  int32_t addr = eval(stmt.args[0].get());
  if (addr < 0 || addr > 0x7F)
  {
    runtime_error(stmt.line, "direccion I2C fuera de rango (0x00-0x7F)");
    return;
  }

  uint8_t reg = has_register ? (uint8_t)eval(stmt.args[1].get()) : 0xFF;
  int32_t bytes = eval(stmt.args[has_register ? 2 : 1].get());

  int32_t max_bytes = little_endian ? 4 : 32;
  if (bytes < 1 || bytes > max_bytes)
  {
    runtime_error(stmt.line, "tamano de lectura invalido (" +
                                 std::to_string(bytes) + "), rango 1-" +
                                 std::to_string(max_bytes));
    return;
  }

  i2c_master_dev_handle_t dev = i2c_get_or_create_device((uint8_t)addr);
  if (!dev)
    return;

  uint8_t reg_buf = reg;
  uint8_t rx_buf[32] = {};

  esp_err_t err = i2c_master_transmit_receive(dev, &reg_buf, 1, rx_buf,
                                              (size_t)bytes,
                                              pdMS_TO_TICKS(100));
  if (err != ESP_OK)
  {
    runtime_error(stmt.line, "error leyendo por I2C: " +
                                 std::string(esp_err_to_name(err)));
    return;
  }

  // Combine up to the first 4 bytes into one integer. Anything wider is
  // still read from the device but only the first 4 bytes form the value.
  int32_t result = 0;
  int32_t combine = bytes > 4 ? 4 : bytes;

  if (little_endian)
  {
    for (int32_t i = combine - 1; i >= 0; i--)
      result = (result << 8) | rx_buf[i];
  }
  else
  {
    for (int32_t i = 0; i < combine; i++)
      result = (result << 8) | rx_buf[i];
  }

  StrideLogger::Log(StrideSubsystem::Interpreter,
                    "I2C read addr=0x%02X reg=0x%02X size=%d -> %s = %d",
                    (unsigned)addr, reg, (int)bytes, stmt.name.c_str(),
                    (int)result);

  _variables[stmt.name] = result;
}
