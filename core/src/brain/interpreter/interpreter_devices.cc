#include "interpreter.hpp"

void Interpreter::clear_devices()
{
  for (auto &[name, device] : _devices)
  {
    delete device.led;
    delete device.buzzer;
    delete device.button;
  }
  _devices.clear();
}

Interpreter::DeviceEntry *Interpreter::find_device(const std::string &name)
{
  auto it = _devices.find(name);
  return (it != _devices.end()) ? &it->second : nullptr;
}

void Interpreter::declare_device(const lang::Stmt &stmt)
{
  using lang::DeviceType;

  DeviceType type = static_cast<DeviceType>(stmt.flags);

  if (type != DeviceType::ExpanderPin && !GPIO_IS_VALID_GPIO((int)stmt.value))
  {
    runtime_error(stmt.line, "el pin " + std::to_string(stmt.value) +
                                 " no es un GPIO valido");
    return;
  }

  DeviceEntry *existing = find_device(stmt.name);
  if (existing)
  {
    delete existing->led;
    delete existing->buzzer;
    delete existing->button;
    _devices.erase(stmt.name);
  }

  DeviceEntry device;
  device.type = type;

  switch (type)
  {
  case DeviceType::Led:
    StrideLogger::Log(StrideSubsystem::Interpreter, "LED %s en pin %u",
                      stmt.name.c_str(), (unsigned)stmt.value);
    device.led = new StrideLed((gpio_num_t)stmt.value);
    break;

  case DeviceType::Buzzer:
    StrideLogger::Log(StrideSubsystem::Interpreter, "BUZZER %s en pin %u",
                      stmt.name.c_str(), (unsigned)stmt.value);
    device.buzzer = new StrideBuzzer((gpio_num_t)stmt.value);
    break;

  case DeviceType::Button:
    StrideLogger::Log(StrideSubsystem::Interpreter, "BUTTON %s en pin %u",
                      stmt.name.c_str(), (unsigned)stmt.value);
    device.button = new StrideButton((gpio_num_t)stmt.value);
    break;

  case DeviceType::ExpanderPin:
    StrideLogger::Log(StrideSubsystem::Interpreter,
                      "Pin %u del expansor -> '%s'", (unsigned)stmt.value,
                      stmt.name.c_str());
    device.expander_pin = (uint8_t)stmt.value;
    break;
  }

  _devices[stmt.name] = device;
}

void Interpreter::device_set(DeviceEntry &device, bool value, uint16_t line)
{
  using lang::DeviceType;

  switch (device.type)
  {
  case DeviceType::Led:
    device.led->set(value);
    break;

  case DeviceType::Buzzer:
    device.buzzer->set(value);
    break;

  case DeviceType::Button:
    runtime_error(line, "un boton es una entrada: no se puede encender "
                        "ni apagar");
    break;

  case DeviceType::ExpanderPin:
  {
    esp_err_t err = expander_pin_write(device.expander_pin, value);
    if (err != ESP_OK)
      runtime_error(line, "error escribiendo el pin del expansor: " +
                              std::string(esp_err_to_name(err)));
    break;
  }
  }
}

int32_t Interpreter::device_get(DeviceEntry &device, uint16_t line)
{
  using lang::DeviceType;

  switch (device.type)
  {
  case DeviceType::Led:
    return device.led->get() ? 1 : 0;

  case DeviceType::Buzzer:
    return device.buzzer->get() ? 1 : 0;

  case DeviceType::Button:
    return device.button->is_pressed() ? 1 : 0;

  case DeviceType::ExpanderPin:
  {
    bool level = false;
    esp_err_t err = expander_pin_read(device.expander_pin, level);
    if (err != ESP_OK)
    {
      runtime_error(line, "error leyendo el pin del expansor: " +
                              std::string(esp_err_to_name(err)));
      return 0;
    }
    return level ? 1 : 0;
  }
  }

  return 0;
}

void Interpreter::exec_turn(const lang::Stmt &stmt)
{
  DeviceEntry *device = find_device(stmt.name);
  if (!device)
  {
    runtime_error(stmt.line, "el dispositivo '" + stmt.name +
                                 "' no esta declarado");
    return;
  }

  device_set(*device, stmt.flags != 0, stmt.line);
  StrideLogger::Log(StrideSubsystem::Interpreter, "turn %s %s",
                    stmt.name.c_str(), stmt.flags ? "on" : "off");
}

void Interpreter::exec_toggle(const lang::Stmt &stmt)
{
  DeviceEntry *device = find_device(stmt.name);
  if (!device)
  {
    runtime_error(stmt.line, "el dispositivo '" + stmt.name +
                                 "' no esta declarado");
    return;
  }

  bool current = device_get(*device, stmt.line) != 0;
  device_set(*device, !current, stmt.line);
}
