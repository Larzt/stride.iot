#include "interpreter.hpp"

int32_t Interpreter::eval(const lang::Expr *expr)
{
  using Kind = lang::Expr::Kind;

  if (!expr)
    return 0;

  switch (expr->kind)
  {
  case Kind::Number:
    return expr->number;

  case Kind::String:
    // The parser only allows strings as print/show arguments.
    return 0;

  case Kind::Name:
  {
    DeviceEntry *device = find_device(expr->text);
    if (device)
      return device_get(*device, expr->line);

    auto it = _variables.find(expr->text);
    if (it != _variables.end())
      return it->second;

    runtime_error(expr->line, "la variable '" + expr->text +
                                  "' no existe todavia, uso 0");
    return 0;
  }

  case Kind::Unary:
  {
    int32_t value = eval(expr->lhs.get());
    if (expr->op == lang::TokKind::KwNot)
      return value == 0 ? 1 : 0;
    return -value; // unary minus
  }

  case Kind::Signed16:
  {
    int32_t value = eval(expr->lhs.get()) & 0xFFFF;
    if (value > 32767)
      value -= 65536;
    return value;
  }

  case Kind::Binary:
  {
    // Short-circuit the logical operators.
    if (expr->op == lang::TokKind::KwAnd)
    {
      if (eval(expr->lhs.get()) == 0)
        return 0;
      return eval(expr->rhs.get()) != 0 ? 1 : 0;
    }
    if (expr->op == lang::TokKind::KwOr)
    {
      if (eval(expr->lhs.get()) != 0)
        return 1;
      return eval(expr->rhs.get()) != 0 ? 1 : 0;
    }

    int32_t lhs = eval(expr->lhs.get());
    int32_t rhs = eval(expr->rhs.get());

    switch (expr->op)
    {
    case lang::TokKind::Plus:
      return lhs + rhs;
    case lang::TokKind::Minus:
      return lhs - rhs;
    case lang::TokKind::Star:
      return lhs * rhs;
    case lang::TokKind::Slash:
      if (rhs == 0)
      {
        runtime_error(expr->line, "division entre cero, uso 0");
        return 0;
      }
      return lhs / rhs;
    case lang::TokKind::Percent:
      if (rhs == 0)
      {
        runtime_error(expr->line, "modulo entre cero, uso 0");
        return 0;
      }
      return lhs % rhs;
    case lang::TokKind::Shl:
      return lhs << rhs;
    case lang::TokKind::Shr:
      return lhs >> rhs;
    case lang::TokKind::Amp:
      return lhs & rhs;
    case lang::TokKind::Pipe:
      return lhs | rhs;
    case lang::TokKind::Eq:
      return lhs == rhs ? 1 : 0;
    case lang::TokKind::Neq:
      return lhs != rhs ? 1 : 0;
    case lang::TokKind::Lt:
      return lhs < rhs ? 1 : 0;
    case lang::TokKind::Le:
      return lhs <= rhs ? 1 : 0;
    case lang::TokKind::Gt:
      return lhs > rhs ? 1 : 0;
    case lang::TokKind::Ge:
      return lhs >= rhs ? 1 : 0;
    default:
      runtime_error(expr->line, "operador desconocido");
      return 0;
    }
  }
  }

  return 0;
}
