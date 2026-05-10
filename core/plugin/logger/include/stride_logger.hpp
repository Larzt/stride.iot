#pragma once

#include "esp_log.h"
#include <cstdio>

enum StrideSubsystem
{
  None,
  Network,
  Server,
  Card,
  Interpreter,
  Screen,
};

class StrideLogger
{
public:
  template <typename... Args>
  static void Log(StrideSubsystem sub, const char *format, Args... args)
  {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), format, args...);
    ESP_LOGI(get_tag(sub), "%s", buffer);
  }

  template <typename... Args>
  static void Error(StrideSubsystem sub, const char *format, Args... args)
  {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), format, args...);
    ESP_LOGE(get_tag(sub), "%s", buffer);
  }

  template <typename... Args>
  static void Warning(StrideSubsystem sub, const char *format, Args... args)
  {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), format, args...);
    ESP_LOGW(get_tag(sub), "%s", buffer);
  }

private:
  StrideLogger() = default;

  static constexpr const char *get_tag(StrideSubsystem subsystem)
  {
    switch (subsystem)
    {
    case StrideSubsystem::Network:
      return "STRIDE_NETWORK";
    case StrideSubsystem::Server:
      return "STRIDE_SERVER";
    case StrideSubsystem::Card:
      return "STRIDE_SDCARD";
    default:
      return "STRIDE";
    }
  }
};
