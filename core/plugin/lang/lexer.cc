#include "lang_lexer.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <unordered_map>

namespace lang
{

static const std::unordered_map<std::string, TokKind> keywords = {
    {"led", TokKind::KwLed},
    {"button", TokKind::KwButton},
    {"buzzer", TokKind::KwBuzzer},
    {"pin", TokKind::KwPin},
    {"turn", TokKind::KwTurn},
    {"toggle", TokKind::KwToggle},
    {"set", TokKind::KwSet},
    {"wait", TokKind::KwWait},
    {"print", TokKind::KwPrint},
    {"show", TokKind::KwShow},
    {"log", TokKind::KwLog},
    {"stop", TokKind::KwStop},
    {"if", TokKind::KwIf},
    {"else", TokKind::KwElse},
    {"end", TokKind::KwEnd},
    {"repeat", TokKind::KwRepeat},
    {"forever", TokKind::KwForever},
    {"while", TokKind::KwWhile},
    {"until", TokKind::KwUntil},
    {"when", TokKind::KwWhen},
    {"every", TokKind::KwEvery},
    {"i2c", TokKind::KwI2c},
    {"on", TokKind::KwOn},
    {"off", TokKind::KwOff},
    {"and", TokKind::KwAnd},
    {"or", TokKind::KwOr},
    {"not", TokKind::KwNot},
    {"is", TokKind::KwIs},
    {"pressed", TokKind::KwPressed},
    {"released", TokKind::KwReleased},
    {"signed16", TokKind::KwSigned16},
};

const char *tok_kind_name(TokKind kind)
{
  switch (kind)
  {
  case TokKind::Number: return "Number";
  case TokKind::Decimal: return "Decimal";
  case TokKind::String: return "String";
  case TokKind::Name: return "Name";
  case TokKind::KwLed: return "led";
  case TokKind::KwButton: return "button";
  case TokKind::KwBuzzer: return "buzzer";
  case TokKind::KwPin: return "pin";
  case TokKind::KwTurn: return "turn";
  case TokKind::KwToggle: return "toggle";
  case TokKind::KwSet: return "set";
  case TokKind::KwWait: return "wait";
  case TokKind::KwPrint: return "print";
  case TokKind::KwShow: return "show";
  case TokKind::KwLog: return "log";
  case TokKind::KwStop: return "stop";
  case TokKind::KwIf: return "if";
  case TokKind::KwElse: return "else";
  case TokKind::KwEnd: return "end";
  case TokKind::KwRepeat: return "repeat";
  case TokKind::KwForever: return "forever";
  case TokKind::KwWhile: return "while";
  case TokKind::KwUntil: return "until";
  case TokKind::KwWhen: return "when";
  case TokKind::KwEvery: return "every";
  case TokKind::KwI2c: return "i2c";
  case TokKind::KwOn: return "on";
  case TokKind::KwOff: return "off";
  case TokKind::KwAnd: return "and";
  case TokKind::KwOr: return "or";
  case TokKind::KwNot: return "not";
  case TokKind::KwIs: return "is";
  case TokKind::KwPressed: return "pressed";
  case TokKind::KwReleased: return "released";
  case TokKind::KwSigned16: return "signed16";
  case TokKind::Assign: return "=";
  case TokKind::Plus: return "+";
  case TokKind::Minus: return "-";
  case TokKind::Star: return "*";
  case TokKind::Slash: return "/";
  case TokKind::Percent: return "%";
  case TokKind::Shl: return "<<";
  case TokKind::Shr: return ">>";
  case TokKind::Amp: return "&";
  case TokKind::Pipe: return "|";
  case TokKind::Eq: return "==";
  case TokKind::Neq: return "!=";
  case TokKind::Lt: return "<";
  case TokKind::Le: return "<=";
  case TokKind::Gt: return ">";
  case TokKind::Ge: return ">=";
  case TokKind::LParen: return "(";
  case TokKind::RParen: return ")";
  case TokKind::Eol: return "end-of-line";
  case TokKind::Eof: return "end-of-file";
  default: return "Unknown";
  }
}

namespace
{

class Lexer
{
public:
  explicit Lexer(const std::string &src) : source(src) {}

  std::vector<Token> run()
  {
    while (cursor < source.size())
    {
      char c = source[cursor];

      if (c == '\n')
      {
        emit(TokKind::Eol, "\n");
        cursor++;
        line++;
      }
      else if (std::isspace(static_cast<unsigned char>(c)))
      {
        cursor++;
      }
      else if (c == '#')
      {
        while (cursor < source.size() && source[cursor] != '\n')
          cursor++;
      }
      else if (c == '"')
      {
        consume_string();
      }
      else if (std::isdigit(static_cast<unsigned char>(c)))
      {
        consume_number();
      }
      else if (std::isalpha(static_cast<unsigned char>(c)) || c == '_')
      {
        consume_word();
      }
      else
      {
        consume_operator();
      }
    }

    emit(TokKind::Eol, "\n");
    emit(TokKind::Eof, "");
    return tokens;
  }

private:
  const std::string &source;
  std::vector<Token> tokens;
  size_t cursor = 0;
  uint16_t line = 1;

  void emit(TokKind kind, std::string text, int32_t num = 0)
  {
    tokens.emplace_back(kind, std::move(text), line, num);
  }

  char peek(size_t offset = 1) const
  {
    return (cursor + offset < source.size()) ? source[cursor + offset] : '\0';
  }

  void consume_string()
  {
    uint16_t start_line = line;
    cursor++; // opening quote
    std::string literal;
    bool closed = false;

    while (cursor < source.size() && source[cursor] != '\n')
    {
      if (source[cursor] == '"')
      {
        closed = true;
        cursor++;
        break;
      }
      literal += source[cursor];
      cursor++;
    }

    if (closed)
      tokens.emplace_back(TokKind::String, std::move(literal), start_line);
    else
      tokens.emplace_back(TokKind::Unknown, "\"" + literal, start_line);
  }

  void consume_number()
  {
    std::string word;

    if (source[cursor] == '0' && (peek() == 'x' || peek() == 'X'))
    {
      word += source[cursor];
      word += source[cursor + 1];
      cursor += 2;
      while (cursor < source.size() &&
             std::isxdigit(static_cast<unsigned char>(source[cursor])))
      {
        word += source[cursor];
        cursor++;
      }
      if (word.size() == 2)
      {
        emit(TokKind::Unknown, word);
        return;
      }
      long value = std::strtol(word.c_str() + 2, nullptr, 16);
      emit(TokKind::Number, word, static_cast<int32_t>(value));
      return;
    }

    bool has_dot = false;
    while (cursor < source.size())
    {
      char c = source[cursor];
      if (std::isdigit(static_cast<unsigned char>(c)))
      {
        word += c;
        cursor++;
      }
      else if (c == '.' && !has_dot &&
               std::isdigit(static_cast<unsigned char>(peek())))
      {
        has_dot = true;
        word += c;
        cursor++;
      }
      else
      {
        break;
      }
    }

    // A digit glued to letters ("5s", "12abc") is not valid: the unit goes
    // separated ("5 s"). Consume the trailing letters so the error is whole.
    if (cursor < source.size() &&
        (std::isalpha(static_cast<unsigned char>(source[cursor])) ||
         source[cursor] == '_'))
    {
      while (cursor < source.size() &&
             (std::isalnum(static_cast<unsigned char>(source[cursor])) ||
              source[cursor] == '_'))
      {
        word += source[cursor];
        cursor++;
      }
      emit(TokKind::Unknown, word);
      return;
    }

    if (has_dot)
    {
      emit(TokKind::Decimal, word);
      return;
    }

    long value = std::strtol(word.c_str(), nullptr, 10);
    emit(TokKind::Number, word, static_cast<int32_t>(value));
  }

  void consume_word()
  {
    std::string word;
    while (cursor < source.size() &&
           (std::isalnum(static_cast<unsigned char>(source[cursor])) ||
            source[cursor] == '_'))
    {
      word += source[cursor];
      cursor++;
    }

    std::string lower = word;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    auto it = keywords.find(lower);
    if (it != keywords.end())
      emit(it->second, word);
    else
      emit(TokKind::Name, word);
  }

  void consume_operator()
  {
    char c = source[cursor];
    char n = peek();

    auto two = [&](TokKind kind, const char *text)
    {
      emit(kind, text);
      cursor += 2;
    };
    auto one = [&](TokKind kind, const char *text)
    {
      emit(kind, text);
      cursor++;
    };

    if (c == '<' && n == '<') { two(TokKind::Shl, "<<"); return; }
    if (c == '>' && n == '>') { two(TokKind::Shr, ">>"); return; }
    if (c == '=' && n == '=') { two(TokKind::Eq, "=="); return; }
    if (c == '!' && n == '=') { two(TokKind::Neq, "!="); return; }
    if (c == '<' && n == '=') { two(TokKind::Le, "<="); return; }
    if (c == '>' && n == '=') { two(TokKind::Ge, ">="); return; }
    if (c == '-' && n == '>') { two(TokKind::Unknown, "->"); return; }

    switch (c)
    {
    case '=': one(TokKind::Assign, "="); return;
    case '<': one(TokKind::Lt, "<"); return;
    case '>': one(TokKind::Gt, ">"); return;
    case '+': one(TokKind::Plus, "+"); return;
    case '-': one(TokKind::Minus, "-"); return;
    case '*': one(TokKind::Star, "*"); return;
    case '/': one(TokKind::Slash, "/"); return;
    case '%': one(TokKind::Percent, "%"); return;
    case '&': one(TokKind::Amp, "&"); return;
    case '|': one(TokKind::Pipe, "|"); return;
    case '(': one(TokKind::LParen, "("); return;
    case ')': one(TokKind::RParen, ")"); return;
    default:
      one(TokKind::Unknown, std::string(1, c).c_str());
      return;
    }
  }
};

} // namespace

std::vector<Token> lex(const std::string &source)
{
  return Lexer(source).run();
}

} // namespace lang
