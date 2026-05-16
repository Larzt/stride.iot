#pragma once

#include <string>
#include <vector>
#include <iostream>

enum class TokenType
{

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

  IF,
  ELSE,
  ENDIF,
  LOOP,
  DLOOP,
  WAIT,

  I2C,
  INIT,
  READLE,
  SDA,
  SCL,

  SIGN16,

  STRING,
  NUMBER,
  HEX_NUMBER,
  IDENTIFIER,
  VALUE,

  ADD,
  SUB,
  MUL,
  DIV,
  MOD,
  SHL,
  SHR,
  BIT_AND,
  BIT_OR,

  ARROW,
  ASSIGN,
  LPAREN,
  RPAREN,

  IS_EQUAL,
  NOT_EQUAL,
  LESS_EQUAL,
  GREATER_EQUAL,
  LESS_THAN,
  GREATER_THAN,

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
    case TokenType::FILE:
      return "FILE";
    case TokenType::DEVICE:
      return "DEVICE";
    case TokenType::PIN:
      return "PIN";
    case TokenType::BUTTON:
      return "BUTTON";
    case TokenType::BUZZER:
      return "BUZZER";
    case TokenType::LED:
      return "LED";
    case TokenType::NAME:
      return "NAME";
    case TokenType::WRITE:
      return "WRITE";
    case TokenType::READ:
      return "READ";
    case TokenType::PRINT:
      return "PRINT";
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
    case TokenType::I2C:
      return "I2C";
    case TokenType::INIT:
      return "INIT";
    case TokenType::READLE:
      return "READLE";
    case TokenType::SDA:
      return "SDA";
    case TokenType::SCL:
      return "SCL";
    case TokenType::SIGN16:
      return "SIGN16";
    case TokenType::STRING:
      return "STRING";
    case TokenType::NUMBER:
      return "NUMBER";
    case TokenType::HEX_NUMBER:
      return "HEX_NUMBER";
    case TokenType::IDENTIFIER:
      return "IDENTIFIER";
    case TokenType::VALUE:
      return "VALUE";
    case TokenType::ADD:
      return "ADD";
    case TokenType::SUB:
      return "SUB";
    case TokenType::MUL:
      return "MUL";
    case TokenType::DIV:
      return "DIV";
    case TokenType::MOD:
      return "MOD";
    case TokenType::SHL:
      return "SHL";
    case TokenType::SHR:
      return "SHR";
    case TokenType::BIT_AND:
      return "BIT_AND";
    case TokenType::BIT_OR:
      return "BIT_OR";
    case TokenType::ARROW:
      return "ARROW";
    case TokenType::ASSIGN:
      return "ASSIGN";
    case TokenType::LPAREN:
      return "LPAREN";
    case TokenType::RPAREN:
      return "RPAREN";
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
      return "UNKNOWN";
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
