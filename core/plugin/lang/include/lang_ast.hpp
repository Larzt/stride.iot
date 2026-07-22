#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "lang_token.hpp"

namespace lang
{

// ---------------------------------------------------------------------------
// Expressions
// ---------------------------------------------------------------------------

struct Expr;
using ExprPtr = std::unique_ptr<Expr>;

struct Expr
{
  enum class Kind : uint8_t
  {
    Number,    // literal -> number
    String,    // literal -> text (only as print/show argument)
    Name,      // variable or device read -> text
    Unary,     // op (KwNot | Minus) applied to lhs
    Binary,    // op applied to lhs, rhs
    Signed16,  // signed16(lhs)
  };

  Kind kind;
  uint16_t line = 0;
  int32_t number = 0;
  std::string text;
  TokKind op = TokKind::Unknown;
  ExprPtr lhs;
  ExprPtr rhs;

  explicit Expr(Kind k, uint16_t l) : kind(k), line(l) {}
};

// ---------------------------------------------------------------------------
// Statements
// ---------------------------------------------------------------------------

struct Stmt;
using Block = std::vector<Stmt>;

// Device types stored in Stmt::flags for DeclareDevice.
enum class DeviceType : uint8_t
{
  Led = 0,
  Buzzer = 1,
  Button = 2,
  ExpanderPin = 3,
};

// Repeat modes stored in Stmt::flags for Repeat.
enum class RepeatMode : uint8_t
{
  Times = 0,
  Forever = 1,
  While = 2,
  Until = 3,
};

struct Stmt
{
  enum class Kind : uint8_t
  {
    DeclareDevice,  // name, flags=DeviceType, value=gpio/expander pin
    Turn,           // name, flags=1 on / 0 off
    Toggle,         // name
    Set,            // name, args[0]=value expression
    Wait,           // flags=0: value=milliseconds
                    // flags=1: args[0]=magnitude expr, value=unit factor (ms)
    Print,          // args = strings/expressions to concatenate
    Show,           // args = strings/expressions to concatenate
    LogTo,          // name = log filename
    Stop,           // ends the program
    If,             // branches = {cond, body}; else branch has null cond
    Repeat,         // flags=RepeatMode, args[0]=count/cond expr, body
    When,           // name = button, flags=1 pressed / 0 released, body
    Every,          // like Wait for the period, body
    I2cWrite,       // args=[addr, (reg), value], flags bit1 = has register
    I2cRead,        // args=[addr, (reg), size], name = destination variable,
                    // flags bit0 = little endian, bit1 = has register
  };

  Kind kind;
  uint16_t line = 0;
  std::string name;
  uint8_t flags = 0;
  uint32_t value = 0;
  std::vector<ExprPtr> args;
  std::vector<std::pair<ExprPtr, Block>> branches;
  Block body;

  explicit Stmt(Kind k, uint16_t l) : kind(k), line(l) {}

  Stmt(Stmt &&) = default;
  Stmt &operator=(Stmt &&) = default;
  Stmt(const Stmt &) = delete;
  Stmt &operator=(const Stmt &) = delete;
};

struct Program
{
  Block top;
};

} // namespace lang
