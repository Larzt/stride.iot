#pragma once
#include <map>
#include <string>

#include "lang_parser.hpp"
#include "enums.hpp"

using StrideProgram = lang::Program;

template <typename T>
using StrideVariable = std::map<std::string, T>;

struct InputEvent
{
  InputType type;
  int delta;
};
