#pragma once

#include <string>
#include <functional>
#include "enums.hpp"

struct AppDescriptor
{
  AppType type = AppType::Script;
  std::string name;
  std::string path;
  std::function<void()> action;

  bool operator==(const AppDescriptor &other) const
  {
    return name == other.name && path == other.path;
  }

  bool operator!=(const AppDescriptor &other) const
  {
    return !(*this == other);
  }
};
