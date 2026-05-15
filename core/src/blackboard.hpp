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

  // Network
  static inline std::string AccessSSID = "Stride";
  static inline std::string AccessPASS = "12345678";
  static inline int MaxNetConnections = 4;

  static inline bool IsNetConnected = false;
  static inline StrideObservable<std::string> WifiIpAddress{"0.0.0.0"};
  static inline StrideObservable<std::string> LocalIpAddress{"0.0.0.0"};
  static inline int MaxConnectionRetries = 5;
  static inline StrideObservable<NetworkMode> CurrentNetworkMode{NetworkMode::Access};
  static inline gpio_num_t NetLed = GPIO_NUM_27;

  // Server
  static uint16_t inline Port = 80;
  static uint16_t inline HeaderLength = 1024;
  static uint16_t inline MaxHandlers = 12;
  static inline gpio_num_t ModeLed = GPIO_NUM_27;
  static inline int ModeTimePressed = 5000;
  static inline StrideObservable<ServerMode> CurrentServerMode{ServerMode::Developer};

  // Global buttons
  static inline gpio_num_t LeftButton = GPIO_NUM_32;
  static inline gpio_num_t RightButton = GPIO_NUM_16;
  static inline gpio_num_t EnterButton = GPIO_NUM_33;

  // System
  static inline int MaxSDRetries = 3;
  static inline int ReloadTimePressed = 1000;
  static inline int ReconnectHoldMs = 3000;
  static inline std::string MountPoint = "/sdcard";
  static inline std::string CurrentLogFile = "/prints.log";
  static inline StrideObservable<AppDescriptor> CurrentProgram;
  // See more about this here: https://en.wikipedia.org/wiki/List_of_UTC_offsets
  static inline const char *TimeZone = "TZ";

  // Handlers
  static inline gpio_num_t PingLed = GPIO_NUM_25;

  // Display
  static inline uint32_t LoadingTimeMs = 2000;
  static inline StrideObservable<int> FileListVersion{0};
  static inline StrideObservable<bool> SdCardMounted{false};

  // Defaults
  static void Reset()
  {
    IsNetConnected = false;
    WifiIpAddress = "0.0.0.0";
    LocalIpAddress = "0.0.0.0";
    MaxConnectionRetries = 5;
    Port = 80;
  }
};
