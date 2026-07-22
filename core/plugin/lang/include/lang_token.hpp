#pragma once

#include <cstdint>
#include <string>

namespace lang
{

enum class TokKind : uint8_t
{
  // Literals
  Number,   // integer literal (decimal or hex), parsed value in Token::num
  Decimal,  // decimal literal like 1.5 — only valid as a duration magnitude
  String,   // "..." literal, contents in Token::text
  Name,     // identifier (also contextual words: to, times, register, value,
            // size, into, little, endian, expander, ms, s, min, write, read)

  // Statement keywords
  KwLed,
  KwButton,
  KwBuzzer,
  KwPin,
  KwTurn,
  KwToggle,
  KwSet,
  KwWait,
  KwPrint,
  KwShow,
  KwLog,
  KwStop,
  KwIf,
  KwElse,
  KwEnd,
  KwRepeat,
  KwForever,
  KwWhile,
  KwUntil,
  KwWhen,
  KwEvery,
  KwI2c,

  // Expression keywords
  KwOn,
  KwOff,
  KwAnd,
  KwOr,
  KwNot,
  KwIs,
  KwPressed,
  KwReleased,
  KwSigned16,

  // Operators
  Assign,        // =
  Plus,          // +
  Minus,         // -
  Star,          // *
  Slash,         // /
  Percent,       // %
  Shl,           // <<
  Shr,           // >>
  Amp,           // &
  Pipe,          // |
  Eq,            // ==
  Neq,           // !=
  Lt,            // <
  Le,            // <=
  Gt,            // >
  Ge,            // >=
  LParen,        // (
  RParen,        // )

  Eol,           // end of line
  Eof,           // end of source
  Unknown,       // unrecognized input (reported by the parser)
};

struct Token
{
  TokKind kind = TokKind::Unknown;
  std::string text;     // raw text (identifier/string/keyword as written)
  int32_t num = 0;      // parsed value for Number tokens
  uint16_t line = 0;    // 1-based source line

  Token() = default;
  Token(TokKind k, std::string t, uint16_t l, int32_t n = 0)
      : kind(k), text(std::move(t)), num(n), line(l) {}
};

const char *tok_kind_name(TokKind kind);

} // namespace lang
