#pragma once

#include <string>
#include "enums.hpp"

struct AppDescriptor
{
  AppType type = AppType::Script;
  std::string name;
  std::string path;

  bool operator==(const AppDescriptor &other) const
  {
    return name == other.name && path == other.path;
  }

  bool operator!=(const AppDescriptor &other) const
  {
    return !(*this == other);
  }
};
