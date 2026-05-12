#pragma once
#include <string>

#include "stride_logger.hpp"
#include "blackboard.hpp"

class TimeUtils
{
public:
  static void initialize();
  static std::string get_timestamp();

private:
  static bool _initialized;
};
