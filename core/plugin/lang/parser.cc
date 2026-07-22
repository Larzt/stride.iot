#include "lang_parser.hpp"

#include <algorithm>
#include <cstdlib>
#include <unordered_map>

#include "lang_lexer.hpp"

namespace lang
{

namespace
{

constexpr int kMaxNesting = 16;

// Milliseconds per time-unit word. Unit words are contextual (they are Name
// tokens), so "s" or "min" remain usable as variable names elsewhere.
const std::unordered_map<std::string, uint32_t> time_units = {
    {"ms", 1},
    {"millisecond", 1},
    {"milliseconds", 1},
    {"s", 1000},
    {"sec", 1000},
    {"second", 1000},
    {"seconds", 1000},
    {"min", 60000},
    {"mins", 60000},
    {"minute", 60000},
    {"minutes", 60000},
};

std::string to_lower(const std::string &word)
{
  std::string lower = word;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
  return lower;
}

enum class BlockEnd
{
  End,
  Else,
  Eof,
};

class Parser
{
public:
  explicit Parser(std::vector<Token> toks) : tokens(std::move(toks)) {}

  ParseResult run()
  {
    ParseResult result;

    skip_eols();
    while (!at(TokKind::Eof))
    {
      if (at(TokKind::KwEnd))
      {
        error(cur().line, "este 'end' no cierra ningun bloque");
        sync_line();
      }
      else if (at(TokKind::KwElse))
      {
        error(cur().line, "este 'else' no pertenece a ningun 'if'");
        sync_line();
      }
      else
      {
        parse_statement_into(result.program.top);
      }
      skip_eols();
    }

    std::stable_sort(errors.begin(), errors.end(),
                     [](const ParseError &a, const ParseError &b)
                     { return a.line < b.line; });
    result.errors = std::move(errors);
    return result;
  }

private:
  std::vector<Token> tokens;
  size_t pos = 0;
  int depth = 0;
  uint16_t last_error_line = 0;
  std::vector<ParseError> errors;

  // -- token helpers --------------------------------------------------------

  const Token &cur() const { return tokens[pos]; }
  const Token &peek(size_t offset = 1) const
  {
    size_t i = pos + offset;
    return (i < tokens.size()) ? tokens[i] : tokens.back();
  }

  void next()
  {
    if (pos + 1 < tokens.size())
      pos++;
  }

  bool at(TokKind kind) const { return cur().kind == kind; }

  bool accept(TokKind kind)
  {
    if (!at(kind))
      return false;
    next();
    return true;
  }

  bool at_word(const char *word) const
  {
    return cur().kind == TokKind::Name && to_lower(cur().text) == word;
  }

  bool accept_word(const char *word)
  {
    if (!at_word(word))
      return false;
    next();
    return true;
  }

  bool at_line_end() const
  {
    return at(TokKind::Eol) || at(TokKind::Eof);
  }

  // -- errors ---------------------------------------------------------------

  void error(uint16_t line, std::string message)
  {
    last_error_line = line;
    errors.push_back({line, std::move(message)});
  }

  // Skips the rest of the current line (recovery point after an error).
  void sync_line()
  {
    while (!at_line_end())
      next();
  }

  void skip_eols()
  {
    while (at(TokKind::Eol))
      next();
  }

  // Closes a statement: anything left on the line is an error, unless this
  // line already reported one (avoid noise after recovery).
  void expect_eol(const char *what)
  {
    if (at_line_end())
      return;
    if (cur().line != last_error_line)
      error(cur().line, std::string("sobra texto despues de '") + what +
                            "': no entiendo '" + cur().text + "'");
    sync_line();
  }

  // Reads a user-given name (device or variable). Reserved keywords are
  // rejected with a dedicated message.
  std::string expect_name(const char *what)
  {
    if (at(TokKind::Name))
    {
      std::string name = cur().text;
      next();
      return name;
    }
    if (cur().kind >= TokKind::KwLed && cur().kind <= TokKind::KwSigned16)
      error(cur().line, "'" + cur().text +
                            "' es una palabra reservada del lenguaje y no "
                            "puede usarse como nombre");
    else
      error(cur().line, std::string("falta el nombre de ") + what);
    return "";
  }

  // -- expressions ----------------------------------------------------------

  ExprPtr make_number(int32_t value, uint16_t line)
  {
    auto e = std::make_unique<Expr>(Expr::Kind::Number, line);
    e->number = value;
    return e;
  }

  ExprPtr make_binary(TokKind op, ExprPtr lhs, ExprPtr rhs, uint16_t line)
  {
    if (!lhs || !rhs)
      return nullptr;
    auto e = std::make_unique<Expr>(Expr::Kind::Binary, line);
    e->op = op;
    e->lhs = std::move(lhs);
    e->rhs = std::move(rhs);
    return e;
  }

  ExprPtr parse_expr() { return parse_or(); }

  ExprPtr parse_or()
  {
    ExprPtr lhs = parse_and();
    while (lhs && at(TokKind::KwOr))
    {
      uint16_t line = cur().line;
      next();
      lhs = make_binary(TokKind::KwOr, std::move(lhs), parse_and(), line);
    }
    return lhs;
  }

  ExprPtr parse_and()
  {
    ExprPtr lhs = parse_not();
    while (lhs && at(TokKind::KwAnd))
    {
      uint16_t line = cur().line;
      next();
      lhs = make_binary(TokKind::KwAnd, std::move(lhs), parse_not(), line);
    }
    return lhs;
  }

  ExprPtr parse_not()
  {
    if (at(TokKind::KwNot))
    {
      uint16_t line = cur().line;
      next();
      ExprPtr operand = parse_not();
      if (!operand)
        return nullptr;
      auto e = std::make_unique<Expr>(Expr::Kind::Unary, line);
      e->op = TokKind::KwNot;
      e->lhs = std::move(operand);
      return e;
    }
    return parse_comparison();
  }

  bool at_comparison_op() const
  {
    switch (cur().kind)
    {
    case TokKind::Eq:
    case TokKind::Neq:
    case TokKind::Lt:
    case TokKind::Le:
    case TokKind::Gt:
    case TokKind::Ge:
      return true;
    default:
      return false;
    }
  }

  ExprPtr parse_comparison()
  {
    ExprPtr lhs = parse_bit_or();
    while (lhs && (at_comparison_op() || at(TokKind::KwIs)))
    {
      uint16_t line = cur().line;

      if (accept(TokKind::KwIs))
      {
        // is / is not, with the device states pressed/released as shorthand
        // for == 1 / == 0 (on/off already parse as the literals 1/0).
        TokKind op = accept(TokKind::KwNot) ? TokKind::Neq : TokKind::Eq;
        ExprPtr rhs;
        if (at(TokKind::KwPressed))
        {
          rhs = make_number(1, cur().line);
          next();
        }
        else if (at(TokKind::KwReleased))
        {
          rhs = make_number(0, cur().line);
          next();
        }
        else
        {
          rhs = parse_bit_or();
        }
        lhs = make_binary(op, std::move(lhs), std::move(rhs), line);
        continue;
      }

      TokKind op = cur().kind;
      next();
      lhs = make_binary(op, std::move(lhs), parse_bit_or(), line);
    }
    return lhs;
  }

  ExprPtr parse_bit_or()
  {
    ExprPtr lhs = parse_bit_and();
    while (lhs && at(TokKind::Pipe))
    {
      uint16_t line = cur().line;
      next();
      lhs = make_binary(TokKind::Pipe, std::move(lhs), parse_bit_and(), line);
    }
    return lhs;
  }

  ExprPtr parse_bit_and()
  {
    ExprPtr lhs = parse_shift();
    while (lhs && at(TokKind::Amp))
    {
      uint16_t line = cur().line;
      next();
      lhs = make_binary(TokKind::Amp, std::move(lhs), parse_shift(), line);
    }
    return lhs;
  }

  ExprPtr parse_shift()
  {
    ExprPtr lhs = parse_additive();
    while (lhs && (at(TokKind::Shl) || at(TokKind::Shr)))
    {
      TokKind op = cur().kind;
      uint16_t line = cur().line;
      next();
      lhs = make_binary(op, std::move(lhs), parse_additive(), line);
    }
    return lhs;
  }

  ExprPtr parse_additive()
  {
    ExprPtr lhs = parse_multiplicative();
    while (lhs && (at(TokKind::Plus) || at(TokKind::Minus)))
    {
      TokKind op = cur().kind;
      uint16_t line = cur().line;
      next();
      lhs = make_binary(op, std::move(lhs), parse_multiplicative(), line);
    }
    return lhs;
  }

  ExprPtr parse_multiplicative()
  {
    ExprPtr lhs = parse_unary();
    while (lhs && (at(TokKind::Star) || at(TokKind::Slash) ||
                   at(TokKind::Percent)))
    {
      TokKind op = cur().kind;
      uint16_t line = cur().line;
      next();
      lhs = make_binary(op, std::move(lhs), parse_unary(), line);
    }
    return lhs;
  }

  ExprPtr parse_unary()
  {
    if (at(TokKind::Minus))
    {
      uint16_t line = cur().line;
      next();
      ExprPtr operand = parse_unary();
      if (!operand)
        return nullptr;
      auto e = std::make_unique<Expr>(Expr::Kind::Unary, line);
      e->op = TokKind::Minus;
      e->lhs = std::move(operand);
      return e;
    }
    return parse_primary();
  }

  ExprPtr parse_primary()
  {
    uint16_t line = cur().line;

    switch (cur().kind)
    {
    case TokKind::Number:
    {
      ExprPtr e = make_number(cur().num, line);
      next();
      return e;
    }

    case TokKind::KwOn:
      next();
      return make_number(1, line);

    case TokKind::KwOff:
      next();
      return make_number(0, line);

    case TokKind::Name:
    {
      auto e = std::make_unique<Expr>(Expr::Kind::Name, line);
      e->text = cur().text;
      next();
      return e;
    }

    case TokKind::LParen:
    {
      next();
      ExprPtr inner = parse_expr();
      if (!inner)
        return nullptr;
      if (!accept(TokKind::RParen))
      {
        error(line, "falta el parentesis de cierre ')'");
        return nullptr;
      }
      return inner;
    }

    case TokKind::KwSigned16:
    {
      next();
      if (!accept(TokKind::LParen))
      {
        error(line, "signed16 se usa con parentesis: signed16(valor)");
        return nullptr;
      }
      ExprPtr inner = parse_expr();
      if (!inner)
        return nullptr;
      if (!accept(TokKind::RParen))
      {
        error(line, "falta el parentesis de cierre ')' de signed16");
        return nullptr;
      }
      auto e = std::make_unique<Expr>(Expr::Kind::Signed16, line);
      e->lhs = std::move(inner);
      return e;
    }

    case TokKind::Decimal:
      error(line, "los numeros decimales solo se permiten en tiempos "
                  "(por ejemplo: wait 1.5 s)");
      next();
      return nullptr;

    case TokKind::String:
      error(line, "una cadena de texto no puede usarse dentro de una "
                  "operacion");
      next();
      return nullptr;

    case TokKind::Unknown:
      if (cur().text == "->")
        error(line, "'->' ya no existe: usa 'set variable to valor'");
      else
        error(line, "no reconozco '" + cur().text + "'");
      next();
      return nullptr;

    default:
      error(line, "esperaba un valor y encontre '" + cur().text + "'");
      return nullptr;
    }
  }

  // -- durations ------------------------------------------------------------

  // Returns the milliseconds-per-unit factor, or 0 after reporting an error.
  uint32_t parse_time_unit()
  {
    if (cur().kind == TokKind::Name)
    {
      auto it = time_units.find(to_lower(cur().text));
      if (it != time_units.end())
      {
        next();
        return it->second;
      }
    }
    error(cur().line, "indica la unidad de tiempo: ms, s o min");
    return 0;
  }

  // Parses "<magnitude> <unit>" into st: literal durations (including
  // decimals) become a constant in st.value (flags=0); anything else keeps
  // the expression in st.args[0] with the unit factor in st.value (flags=1).
  void parse_duration(Stmt &st)
  {
    if (at(TokKind::Decimal))
    {
      double magnitude = std::strtod(cur().text.c_str(), nullptr);
      next();
      uint32_t factor = parse_time_unit();
      st.flags = 0;
      st.value = static_cast<uint32_t>(magnitude * factor + 0.5);
      return;
    }

    ExprPtr magnitude = parse_expr();
    if (!magnitude)
      return;

    uint32_t factor = parse_time_unit();

    if (magnitude->kind == Expr::Kind::Number)
    {
      st.flags = 0;
      st.value = (magnitude->number > 0)
                     ? static_cast<uint32_t>(magnitude->number) * factor
                     : 0;
      return;
    }

    st.flags = 1;
    st.value = factor;
    st.args.push_back(std::move(magnitude));
  }

  // -- blocks ---------------------------------------------------------------

  BlockEnd parse_block(Block &out, bool stop_at_else)
  {
    depth++;
    if (depth == kMaxNesting + 1)
      error(cur().line, "demasiados bloques anidados (maximo 16 niveles)");

    skip_eols();
    while (true)
    {
      if (at(TokKind::Eof))
      {
        depth--;
        return BlockEnd::Eof;
      }
      if (at(TokKind::KwEnd))
      {
        next();
        expect_eol("end");
        depth--;
        return BlockEnd::End;
      }
      if (at(TokKind::KwElse))
      {
        if (stop_at_else)
        {
          depth--;
          return BlockEnd::Else;
        }
        error(cur().line, "este 'else' no pertenece a ningun 'if'");
        sync_line();
      }
      else
      {
        parse_statement_into(out);
      }
      skip_eols();
    }
  }

  void missing_end(const char *opener, uint16_t opener_line)
  {
    error(opener_line, std::string("el '") + opener + "' de la linea " +
                           std::to_string(opener_line) +
                           " nunca se cierra: falta su 'end'");
  }

  // -- statements -----------------------------------------------------------

  void parse_statement_into(Block &out)
  {
    uint16_t line = cur().line;

    switch (cur().kind)
    {
    case TokKind::KwLed:
    case TokKind::KwBuzzer:
    case TokKind::KwButton:
      out.push_back(parse_gpio_device());
      return;

    case TokKind::KwPin:
      out.push_back(parse_expander_pin());
      return;

    case TokKind::KwTurn:
      out.push_back(parse_turn());
      return;

    case TokKind::KwToggle:
    {
      next();
      Stmt st(Stmt::Kind::Toggle, line);
      st.name = expect_name("un dispositivo (toggle nombre)");
      expect_eol("toggle");
      out.push_back(std::move(st));
      return;
    }

    case TokKind::KwSet:
    {
      next();
      Stmt st(Stmt::Kind::Set, line);
      st.name = expect_name("la variable (set nombre to valor)");
      if (!accept_word("to"))
        error(cur().line, "falta 'to': set " +
                              (st.name.empty() ? "variable" : st.name) +
                              " to valor");
      else
        st.args.push_back(parse_expr());
      expect_eol("set");
      out.push_back(std::move(st));
      return;
    }

    case TokKind::Name:
    {
      // variable = expression
      if (peek().kind == TokKind::Assign)
      {
        Stmt st(Stmt::Kind::Set, line);
        st.name = cur().text;
        next();
        next();
        st.args.push_back(parse_expr());
        expect_eol("la asignacion");
        out.push_back(std::move(st));
        return;
      }
      error(line, "no entiendo '" + cur().text +
                      "'. Para cambiar una variable escribe: set " +
                      cur().text + " to valor");
      sync_line();
      return;
    }

    case TokKind::KwWait:
    {
      next();
      Stmt st(Stmt::Kind::Wait, line);
      parse_duration(st);
      expect_eol("wait");
      out.push_back(std::move(st));
      return;
    }

    case TokKind::KwPrint:
    case TokKind::KwShow:
    {
      bool is_print = at(TokKind::KwPrint);
      next();
      Stmt st(is_print ? Stmt::Kind::Print : Stmt::Kind::Show, line);
      parse_output_args(st);
      out.push_back(std::move(st));
      return;
    }

    case TokKind::KwLog:
    {
      next();
      Stmt st(Stmt::Kind::LogTo, line);
      if (!accept_word("to"))
        error(cur().line, "escribe: log to \"archivo.log\"");
      else if (at(TokKind::String))
      {
        st.name = cur().text;
        next();
      }
      else
        error(cur().line,
              "falta el nombre del archivo entre comillas: "
              "log to \"archivo.log\"");
      expect_eol("log to");
      out.push_back(std::move(st));
      return;
    }

    case TokKind::KwStop:
    {
      next();
      Stmt st(Stmt::Kind::Stop, line);
      expect_eol("stop");
      out.push_back(std::move(st));
      return;
    }

    case TokKind::KwIf:
      out.push_back(parse_if());
      return;

    case TokKind::KwRepeat:
      out.push_back(parse_repeat());
      return;

    case TokKind::KwWhen:
      out.push_back(parse_when());
      return;

    case TokKind::KwEvery:
      out.push_back(parse_every());
      return;

    case TokKind::KwI2c:
      parse_i2c_into(out);
      return;

    case TokKind::Unknown:
      if (cur().text == "->")
        error(line, "'->' ya no existe: usa 'set variable to valor'");
      else
        error(line, "no reconozco '" + cur().text + "'");
      sync_line();
      return;

    default:
      if (line_contains_arrow())
        error(line, "'->' ya no existe: usa 'set variable to valor'");
      else
        error(line,
              "no entiendo '" + cur().text + "' al principio de la linea");
      sync_line();
      return;
    }
  }

  // Detects the removed "value -> variable" form to give a targeted hint.
  bool line_contains_arrow() const
  {
    for (size_t i = pos; i < tokens.size(); i++)
    {
      if (tokens[i].kind == TokKind::Eol || tokens[i].kind == TokKind::Eof)
        return false;
      if (tokens[i].kind == TokKind::Unknown && tokens[i].text == "->")
        return true;
    }
    return false;
  }

  Stmt parse_gpio_device()
  {
    uint16_t line = cur().line;
    DeviceType type = DeviceType::Led;
    const char *what = "led";
    if (at(TokKind::KwBuzzer))
    {
      type = DeviceType::Buzzer;
      what = "buzzer";
    }
    else if (at(TokKind::KwButton))
    {
      type = DeviceType::Button;
      what = "button";
    }
    next();

    Stmt st(Stmt::Kind::DeclareDevice, line);
    st.flags = static_cast<uint8_t>(type);
    st.name = expect_name("el dispositivo");

    if (!accept(TokKind::KwOn) || !accept(TokKind::KwPin))
      error(cur().line, std::string("la declaracion es: ") + what +
                            " nombre on pin numero");
    else if (at(TokKind::Number))
    {
      st.value = static_cast<uint32_t>(cur().num);
      next();
    }
    else
      error(cur().line, "falta el numero de pin (un numero fijo)");

    expect_eol(what);
    return st;
  }

  Stmt parse_expander_pin()
  {
    uint16_t line = cur().line;
    next();

    Stmt st(Stmt::Kind::DeclareDevice, line);
    st.flags = static_cast<uint8_t>(DeviceType::ExpanderPin);
    st.name = expect_name("el pin del expansor");

    if (!accept(TokKind::KwOn) || !accept_word("expander"))
      error(cur().line, "la declaracion es: pin nombre on expander numero");
    else if (at(TokKind::Number))
    {
      if (cur().num < 0 || cur().num > 7)
        error(cur().line, "el expansor solo tiene los pines 0 a 7");
      else
        st.value = static_cast<uint32_t>(cur().num);
      next();
    }
    else
      error(cur().line, "falta el numero de pin del expansor (0 a 7)");

    expect_eol("pin");
    return st;
  }

  Stmt parse_turn()
  {
    uint16_t line = cur().line;
    next();

    Stmt st(Stmt::Kind::Turn, line);
    st.name = expect_name("un dispositivo (turn nombre on/off)");

    if (accept(TokKind::KwOn))
      st.flags = 1;
    else if (accept(TokKind::KwOff))
      st.flags = 0;
    else
      error(cur().line, "despues de 'turn " +
                            (st.name.empty() ? "nombre" : st.name) +
                            "' di 'on' u 'off'");

    expect_eol("turn");
    return st;
  }

  void parse_output_args(Stmt &st)
  {
    const char *what = (st.kind == Stmt::Kind::Print) ? "print" : "show";

    while (!at_line_end())
    {
      if (at(TokKind::String))
      {
        auto e = std::make_unique<Expr>(Expr::Kind::String, cur().line);
        e->text = cur().text;
        next();
        st.args.push_back(std::move(e));
        continue;
      }

      ExprPtr e = parse_expr();
      if (!e)
      {
        sync_line();
        break;
      }
      st.args.push_back(std::move(e));
    }

    expect_eol(what);
  }

  Stmt parse_if()
  {
    uint16_t line = cur().line;
    next();

    Stmt st(Stmt::Kind::If, line);
    ExprPtr cond = parse_expr();
    if (!cond)
      sync_line();
    expect_eol("la condicion del if");

    Block body;
    BlockEnd end = parse_block(body, true);
    st.branches.emplace_back(std::move(cond), std::move(body));

    while (end == BlockEnd::Else)
    {
      next(); // 'else'

      if (accept(TokKind::KwIf))
      {
        ExprPtr branch_cond = parse_expr();
        if (!branch_cond)
          sync_line();
        expect_eol("la condicion del else if");

        Block branch_body;
        end = parse_block(branch_body, true);
        st.branches.emplace_back(std::move(branch_cond),
                                 std::move(branch_body));
      }
      else
      {
        expect_eol("else");
        Block branch_body;
        end = parse_block(branch_body, false);
        st.branches.emplace_back(nullptr, std::move(branch_body));
        break;
      }
    }

    if (end == BlockEnd::Eof)
      missing_end("if", line);

    return st;
  }

  Stmt parse_repeat()
  {
    uint16_t line = cur().line;
    next();

    Stmt st(Stmt::Kind::Repeat, line);

    if (accept(TokKind::KwForever))
    {
      st.flags = static_cast<uint8_t>(RepeatMode::Forever);
      expect_eol("repeat forever");
    }
    else if (accept(TokKind::KwWhile))
    {
      st.flags = static_cast<uint8_t>(RepeatMode::While);
      st.args.push_back(parse_expr());
      expect_eol("repeat while");
    }
    else if (accept(TokKind::KwUntil))
    {
      st.flags = static_cast<uint8_t>(RepeatMode::Until);
      st.args.push_back(parse_expr());
      expect_eol("repeat until");
    }
    else
    {
      st.flags = static_cast<uint8_t>(RepeatMode::Times);
      ExprPtr count = parse_expr();
      if (!count)
        sync_line();
      else
        st.args.push_back(std::move(count));
      accept_word("times");
      expect_eol("repeat");
    }

    BlockEnd end = parse_block(st.body, false);
    if (end == BlockEnd::Eof)
      missing_end("repeat", line);

    return st;
  }

  Stmt parse_when()
  {
    uint16_t line = cur().line;
    next();

    Stmt st(Stmt::Kind::When, line);
    st.name = expect_name("el boton (when nombre pressed)");

    accept(TokKind::KwIs); // optional: when btn is pressed

    if (accept(TokKind::KwPressed))
      st.flags = 1;
    else if (accept(TokKind::KwReleased))
      st.flags = 0;
    else
      error(cur().line, "despues de 'when " +
                            (st.name.empty() ? "boton" : st.name) +
                            "' di 'pressed' o 'released'");

    expect_eol("when");

    BlockEnd end = parse_block(st.body, false);
    if (end == BlockEnd::Eof)
      missing_end("when", line);

    return st;
  }

  Stmt parse_every()
  {
    uint16_t line = cur().line;
    next();

    Stmt st(Stmt::Kind::Every, line);
    parse_duration(st);
    expect_eol("every");

    BlockEnd end = parse_block(st.body, false);
    if (end == BlockEnd::Eof)
      missing_end("every", line);

    return st;
  }

  void parse_i2c_into(Block &out)
  {
    uint16_t line = cur().line;
    next();

    if (accept_word("write"))
    {
      Stmt st(Stmt::Kind::I2cWrite, line);
      st.args.push_back(parse_expr()); // address

      if (accept_word("register"))
      {
        st.flags |= 2;
        st.args.push_back(parse_expr());
      }

      if (accept_word("value"))
        st.args.push_back(parse_expr());
      else
        error(cur().line,
              "falta 'value': i2c write direccion [register reg] value dato");

      expect_eol("i2c write");
      out.push_back(std::move(st));
      return;
    }

    if (accept_word("read"))
    {
      Stmt st(Stmt::Kind::I2cRead, line);
      st.args.push_back(parse_expr()); // address

      if (accept_word("register"))
      {
        st.flags |= 2;
        st.args.push_back(parse_expr());
      }

      if (accept_word("size"))
        st.args.push_back(parse_expr());
      else
        error(cur().line, "falta 'size': i2c read direccion [register reg] "
                          "size bytes into variable");

      if (accept_word("into"))
        st.name = expect_name("la variable destino");
      else
        error(cur().line, "falta 'into variable' para guardar la lectura");

      if (accept_word("little"))
      {
        st.flags |= 1;
        accept_word("endian");
      }

      expect_eol("i2c read");
      out.push_back(std::move(st));
      return;
    }

    error(cur().line, "despues de 'i2c' escribe 'write' o 'read'");
    sync_line();
  }
};

} // namespace

ParseResult parse(const std::string &source)
{
  return Parser(lex(source)).run();
}

} // namespace lang
