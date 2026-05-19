#include "interpreter.hpp"

static bool is_binary_op(TokenType t)
{
  switch (t)
  {
  case TokenType::ADD: case TokenType::SUB:
  case TokenType::MUL: case TokenType::DIV: case TokenType::MOD:
  case TokenType::SHL: case TokenType::SHR:
  case TokenType::BIT_AND: case TokenType::BIT_OR:
    return true;
  default:
    return false;
  }
}

static int apply_op(int lhs, TokenType op, int rhs)
{
  switch (op)
  {
  case TokenType::ADD:     return lhs + rhs;
  case TokenType::SUB:     return lhs - rhs;
  case TokenType::MUL:     return lhs * rhs;
  case TokenType::DIV:     return rhs != 0 ? lhs / rhs : 0;
  case TokenType::MOD:     return rhs != 0 ? lhs % rhs : 0;
  case TokenType::SHL:     return lhs << rhs;
  case TokenType::SHR:     return lhs >> rhs;
  case TokenType::BIT_AND: return lhs & rhs;
  case TokenType::BIT_OR:  return lhs | rhs;
  default:                 return lhs;
  }
}

int Interpreter::resolve_expr_token(const Token &token)
{
  if (token.type == TokenType::NUMBER)
    return std::stoi(token.value);

  if (token.type == TokenType::HEX_NUMBER)
    return parse_hex_number(token.value);

  if (token.type == TokenType::IDENTIFIER)
  {
    auto it = _variables.find(token.value);
    if (it != _variables.end())
      return it->second;

    StrideLogger::Warning(StrideSubsystem::Interpreter, "Variable '%s' no encontrada, usando 0", token.value.c_str());
    return 0;
  }

  return 0;
}

int Interpreter::eval_primary(const std::vector<Token> &tokens, size_t &pos)
{
  if (pos >= tokens.size())
    return 0;

  if (tokens[pos].type == TokenType::LPAREN)
  {
    pos++;
    int result = eval_expr(tokens, pos);
    if (pos < tokens.size() && tokens[pos].type == TokenType::RPAREN)
      pos++;
    return result;
  }

  return resolve_expr_token(tokens[pos++]);
}

int Interpreter::eval_expr(const std::vector<Token> &tokens, size_t &pos)
{
  int result = eval_primary(tokens, pos);

  while (pos < tokens.size() && is_binary_op(tokens[pos].type))
  {
    TokenType op = tokens[pos++].type;
    int rhs = eval_primary(tokens, pos);
    result = apply_op(result, op, rhs);
  }

  return result;
}

void Interpreter::execute_arrow_allocation(const std::vector<Token> &tokens)
{
  if (tokens.size() < 3)
  {
    StrideLogger::Error(StrideSubsystem::Interpreter, "Expected <value> -> <var>");
    return;
  }

  std::string variable_name = "";
  int variable_value = 0;

  for (size_t i = 0; i < tokens.size(); i++)
  {
    if (tokens[i].type == TokenType::ARROW && i + 1 < tokens.size())
    {
      variable_name = tokens[i + 1].value;

      const Token &val_token = tokens[i - 1];
      if (val_token.type == TokenType::NUMBER)
        variable_value = std::stoi(val_token.value);
      else if (val_token.type == TokenType::HEX_NUMBER)
        variable_value = parse_hex_number(val_token.value);
      else
      {
        StrideLogger::Error(StrideSubsystem::Interpreter,
                            "Invalid value before ->: '%s'", val_token.value.c_str());
        return;
      }
    }
  }

  if (variable_name.empty())
  {
    StrideLogger::Error(StrideSubsystem::Interpreter, "Missing variable name after ->");
    return;
  }

  _variables[variable_name] = variable_value;
}

void Interpreter::execute_simple_allocation(const std::vector<Token> &tokens)
{
  if (tokens.size() < 3)
  {
    StrideLogger::Error(StrideSubsystem::Interpreter, "Expected <var> = <value>");
    return;
  }

  std::string variable_name = tokens[0].value;
  int variable_value = resolve_expr_token(tokens[2]);
  _variables[variable_name] = variable_value;
}

void Interpreter::execute_expr_allocation(const std::vector<Token> &tokens)
{
  if (tokens.size() < 3)
    return;

  std::string var_name = tokens[0].value;
  size_t pos = 2;
  int result = eval_expr(tokens, pos);

  _variables[var_name] = result;
  StrideLogger::Log(StrideSubsystem::Interpreter, "%s = %d", var_name.c_str(), result);
}

void Interpreter::execute_sign16_command(const std::vector<Token> &tokens)
{
  if (tokens.size() < 2 || tokens[1].type != TokenType::IDENTIFIER)
  {
    StrideLogger::Error(StrideSubsystem::Interpreter, "SIGN16: sintaxis invalida. Uso: SIGN16 <var>");
    return;
  }

  const std::string &name = tokens[1].value;
  auto it = _variables.find(name);
  if (it == _variables.end())
  {
    StrideLogger::Warning(StrideSubsystem::Interpreter, "SIGN16: variable '%s' no encontrada", name.c_str());
    return;
  }

  if (it->second > 32767)
    it->second -= 65536;

  StrideLogger::Log(StrideSubsystem::Interpreter, "SIGN16 %s = %d", name.c_str(), it->second);
}
