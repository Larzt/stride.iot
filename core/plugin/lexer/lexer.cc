#include "lexer.hpp"
#include "liner.hpp"

std::vector<Token> tokenize(const std::string &line)
{
  Liner liner(line);
  return liner.process();
}

std::ostream &operator<<(std::ostream &out, TokenType type)
{
  switch (type)
  {
  case TokenType::FILE:          out << "FILE";          break;
  case TokenType::DEVICE:        out << "DEVICE";        break;
  case TokenType::PIN:           out << "PIN";           break;
  case TokenType::BUTTON:        out << "BUTTON";        break;
  case TokenType::BUZZER:        out << "BUZZER";        break;
  case TokenType::LED:           out << "LED";           break;
  case TokenType::NAME:          out << "NAME";          break;
  case TokenType::WRITE:         out << "WRITE";         break;
  case TokenType::READ:          out << "READ";          break;
  case TokenType::PRINT:         out << "PRINT";         break;
  case TokenType::IF:            out << "IF";            break;
  case TokenType::ELSE:          out << "ELSE";          break;
  case TokenType::ENDIF:         out << "ENDIF";         break;
  case TokenType::LOOP:          out << "LOOP";          break;
  case TokenType::DLOOP:         out << "DLOOP";         break;
  case TokenType::WAIT:          out << "WAIT";          break;
  case TokenType::I2C:           out << "I2C";           break;
  case TokenType::INIT:          out << "INIT";          break;
  case TokenType::READLE:        out << "READLE";        break;
  case TokenType::SDA:           out << "SDA";           break;
  case TokenType::SCL:           out << "SCL";           break;
  case TokenType::SIGN16:        out << "SIGN16";        break;
  case TokenType::STRING:        out << "STRING";        break;
  case TokenType::NUMBER:        out << "NUMBER";        break;
  case TokenType::HEX_NUMBER:    out << "HEX_NUMBER";    break;
  case TokenType::IDENTIFIER:    out << "IDENTIFIER";    break;
  case TokenType::VALUE:         out << "VALUE";         break;
  case TokenType::ADD:           out << "ADD";           break;
  case TokenType::SUB:           out << "SUB";           break;
  case TokenType::MUL:           out << "MUL";           break;
  case TokenType::DIV:           out << "DIV";           break;
  case TokenType::MOD:           out << "MOD";           break;
  case TokenType::SHL:           out << "SHL";           break;
  case TokenType::SHR:           out << "SHR";           break;
  case TokenType::BIT_AND:       out << "BIT_AND";       break;
  case TokenType::BIT_OR:        out << "BIT_OR";        break;
  case TokenType::ARROW:         out << "ARROW";         break;
  case TokenType::ASSIGN:        out << "ASSIGN";        break;
  case TokenType::LPAREN:        out << "LPAREN";        break;
  case TokenType::RPAREN:        out << "RPAREN";        break;
  case TokenType::IS_EQUAL:      out << "IS_EQUAL";      break;
  case TokenType::NOT_EQUAL:     out << "NOT_EQUAL";     break;
  case TokenType::LESS_EQUAL:    out << "LESS_EQUAL";    break;
  case TokenType::GREATER_EQUAL: out << "GREATER_EQUAL"; break;
  case TokenType::LESS_THAN:     out << "LESS_THAN";     break;
  case TokenType::GREATER_THAN:  out << "GREATER_THAN";  break;
  default:                       out << "UNKNOWN";       break;
  }
  return out;
}

std::ostream &operator<<(std::ostream &out, const Token &token)
{
  out << token.type << ": " << token.value;
  return out;
}
