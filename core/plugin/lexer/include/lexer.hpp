#pragma once

#include <string>
#include <vector>
#include <iostream>

enum class TokenType
{
  // BASICS
  FILE,
  DEVICE,
  PIN,
  BUTTON,
  BUZZER,
  LED,
  NAME,
  WRITE,
  READ,
  PRINT,

  // FLOW CONTROL
  IF,
  ELSE,
  ENDIF,
  LOOP,
  DLOOP,
  WAIT,

  // I2C
  I2C,
  INIT,
  SDA,
  SCL,

  // LITERALS
  STRING,
  NUMBER,
  HEX_NUMBER,
  IDENTIFIER,
  VALUE,

  // OPERADORES ARITMÉTICOS
  ADD,     // +
  SUB,     // -
  MUL,     // *
  DIV,     // /
  MOD,     // %
  SHL,     //
  SHR,     // >>
  BIT_AND, // &
  BIT_OR,  // |

  // SYMBOLS
  ARROW,
  ASSIGN,

  // CONDITIONALS
  IS_EQUAL,      // ==
  NOT_EQUAL,     // !=
  LESS_EQUAL,    // <=
  GREATER_EQUAL, // >=
  LESS_THAN,     // <
  GREATER_THAN,  // >

  UNKNOWN,
};

struct Token
{
  TokenType type;
  std::string value;

  const char *get_value() const { return value.c_str(); }
  const char *get_type() const
  {
    switch (type)
    {
    case TokenType::LED:
      return "LED";
    case TokenType::BUZZER:
      return "BUZZER";
    case TokenType::BUTTON:
      return "BUTTON";
    case TokenType::PIN:
      return "PIN";
    case TokenType::NAME:
      return "NAME";
    case TokenType::WRITE:
      return "WRITE";
    case TokenType::READ:
      return "READ";
    case TokenType::PRINT:
      return "PRINT";
    case TokenType::NUMBER:
      return "NUMBER";
    case TokenType::IDENTIFIER:
      return "IDENTIFIER";
    case TokenType::VALUE:
      return "VALUE";
    case TokenType::STRING:
      return "STRING";

    // CONTROL DE FLUJO
    case TokenType::IF:
      return "IF";
    case TokenType::ELSE:
      return "ELSE";
    case TokenType::ENDIF:
      return "ENDIF";
    case TokenType::LOOP:
      return "LOOP";
    case TokenType::DLOOP:
      return "DLOOP";
    case TokenType::WAIT:
      return "WAIT";

    // OPERADORES / ASIGNACIÓN
    case TokenType::ASSIGN:
      return "ASSIGN";
    case TokenType::ARROW:
      return "ARROW";

    // CONDICIONALES
    case TokenType::IS_EQUAL:
      return "IS_EQUAL";
    case TokenType::NOT_EQUAL:
      return "NOT_EQUAL";
    case TokenType::LESS_EQUAL:
      return "LESS_EQUAL";
    case TokenType::GREATER_EQUAL:
      return "GREATER_EQUAL";
    case TokenType::LESS_THAN:
      return "LESS_THAN";
    case TokenType::GREATER_THAN:
      return "GREATER_THAN";

    default:
      return "UNKNOWN_TOKEN";
    }
  }

  Token() : type(TokenType::UNKNOWN), value("") {}

  Token(const Token &token) = default;
  Token(TokenType t) : type(t), value("") {}
  Token(TokenType t, const std::string &v) : type(t), value(v) {}
};

std::vector<Token> tokenize(const std::string &line);

std::ostream &operator<<(std::ostream &out, TokenType type);
std::ostream &operator<<(std::ostream &out, const Token &token);
