#pragma once
#include <vector>
#include <map>

#include "lexer.hpp"
#include "enums.hpp"

using StrideProgram = std::vector<std::vector<Token>>;

template <typename T>
using StrideVariable = std::map<std::string, T>;

struct InputEvent
{
  InputType type;
  int delta;
};
