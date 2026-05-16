#pragma once

enum class NetworkMode
{
  Access,
  Station,
};

enum class ServerMode
{
  Developer,
  Production
};

enum class StateType {
  Startup,
  Main,
  View,
  Running,
};

enum class InputType
{
  Button,
  Encoder,
  Touch
};

enum class AppType {
  Builtin,
  Script,
};
