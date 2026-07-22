#pragma once

#include <string>
#include <vector>

#include "lang_ast.hpp"

namespace lang
{

struct ParseError
{
  uint16_t line;
  std::string message;
};

struct ParseResult
{
  Program program;
  std::vector<ParseError> errors;

  bool ok() const { return errors.empty(); }
};

// Parses a whole .str source file. All errors are collected (with their line
// number); a program with errors must not be executed.
ParseResult parse(const std::string &source);

} // namespace lang
