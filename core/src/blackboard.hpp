#pragma once
#include <cstdint>
#include <string>
#include <driver/gpio.h>

#include "enums.hpp"
#include "app.hpp"
#include "stride_observer.hpp"

struct Blackboard
{
  Blackboard() = delete;

  static inline std::string AccessSSID = "Stride";
  static inline std::string AccessPASS = "12345678";
  static inline int MaxNetConnections = 4;

  static inline bool IsNetConnected = false;
  static inline StrideObservable<std::string> WifiIpAddress{"0.0.0.0"};
  static inline StrideObservable<std::string> LocalIpAddress{"0.0.0.0"};
  static inline int MaxConnectionRetries = 5;
  static inline StrideObservable<NetworkMode> CurrentNetworkMode{NetworkMode::Access};
  static inline gpio_num_t NetLed = GPIO_NUM_27;

  static uint16_t inline Port = 80;
  static uint16_t inline HeaderLength = 1024;
  static uint16_t inline MaxHandlers = 16;
  static inline gpio_num_t ModeLed = GPIO_NUM_27;
  static inline int ModeTimePressed = 5000;
  static inline StrideObservable<ServerMode> CurrentServerMode{ServerMode::Developer};

  static inline gpio_num_t LeftButton = GPIO_NUM_32;
  static inline gpio_num_t RightButton = GPIO_NUM_16;
  static inline gpio_num_t EnterButton = GPIO_NUM_33;

  static inline int MaxSDRetries = 3;
  static inline int ReloadTimePressed = 1000;
  static inline std::string MountPoint = "/sdcard";
  static inline std::string CurrentLogFile = "/prints.log";
  static inline StrideObservable<AppDescriptor> CurrentProgram;

  static inline const char *TimeZone = "TZ";

  static inline gpio_num_t PingLed = GPIO_NUM_25;

  static inline uint32_t LoadingTimeMs = 2000;
  static inline StrideObservable<int> FileListVersion{0};
  static inline StrideObservable<bool> SdCardMounted{false};

  static inline StrideObservable<std::string> RunningProgramName{""};

  // Last text emitted by the DSL `show` command (rendered by RunningState).
  static inline StrideObservable<std::string> DslShowText{""};

  static void Reset()
  {
    IsNetConnected = false;
    WifiIpAddress = "0.0.0.0";
    LocalIpAddress = "0.0.0.0";
    MaxConnectionRetries = 5;
    Port = 80;
  }
};
