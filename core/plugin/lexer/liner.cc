#include "liner.hpp"

#include <unordered_map>
#include <algorithm>
#include <cctype>

static const std::unordered_map<std::string, TokenType> keywords = {
    {"file", TokenType::FILE},
    {"device", TokenType::DEVICE},
    {"led", TokenType::LED},
    {"buzzer", TokenType::BUZZER},
    {"button", TokenType::BUTTON},
    {"pin", TokenType::PIN},
    {"name", TokenType::NAME},
    {"write", TokenType::WRITE},
    {"read", TokenType::READ},
    {"print", TokenType::PRINT},

    {"if", TokenType::IF},
    {"else", TokenType::ELSE},
    {"endif", TokenType::ENDIF},
    {"loop", TokenType::LOOP},
    {"dloop", TokenType::DLOOP},
    {"wait", TokenType::WAIT},

    {"i2c", TokenType::I2C},
    {"init", TokenType::INIT},
    {"readle", TokenType::READLE},
    {"sda", TokenType::SDA},
    {"scl", TokenType::SCL},
    {"sign16", TokenType::SIGN16},

    {"->", TokenType::ARROW},
    {"=", TokenType::ASSIGN},
    {"on", TokenType::VALUE},
    {"off", TokenType::VALUE}};

Liner::Liner(const std::string &line) : source(line)
{
}

std::vector<Token> Liner::process()
{
  while (cursor < source.length())
  {
    char current_char = source[cursor];
    char next_char = peek_next();

    if (std::isspace(current_char))
    {
      flush_word();
      cursor++;
    }
    else if (current_char == '"')
    {
      flush_word();
      consume_string();
    }

    else if (current_char == '-' && next_char == '>')
    {
      flush_word();
      tokens.emplace_back(TokenType::ARROW, "->");
      cursor += 2;
    }
    else if (current_char == '<' && next_char == '<')
    {
      flush_word();
      tokens.emplace_back(TokenType::SHL, "<<");
      cursor += 2;
    }
    else if (current_char == '>' && next_char == '>')
    {
      flush_word();
      tokens.emplace_back(TokenType::SHR, ">>");
      cursor += 2;
    }
    else if (current_char == '=' && next_char == '=')
    {
      flush_word();
      char third_char = (cursor + 2 < source.length()) ? source[cursor + 2] : '\0';

      if (third_char == '=')
      {

        std::string invalid_op = "==";
        cursor += 2;
        while (cursor < source.length() && source[cursor] == '=')
        {
          invalid_op += source[cursor];
          cursor++;
        }
        tokens.emplace_back(TokenType::UNKNOWN, invalid_op);
      }
      else
      {
        tokens.emplace_back(TokenType::IS_EQUAL, "==");
        cursor += 2;
      }
    }
    else if (current_char == '!' && next_char == '=')
    {
      flush_word();
      tokens.emplace_back(TokenType::NOT_EQUAL, "!=");
      cursor += 2;
    }
    else if (current_char == '<' && next_char == '=')
    {
      flush_word();
      tokens.emplace_back(TokenType::LESS_EQUAL, "<=");
      cursor += 2;
    }
    else if (current_char == '>' && next_char == '=')
    {
      flush_word();
      tokens.emplace_back(TokenType::GREATER_EQUAL, ">=");
      cursor += 2;
    }

    else if (current_char == '=')
    {
      flush_word();
      tokens.emplace_back(TokenType::ASSIGN, "=");
      cursor++;
    }
    else if (current_char == '<')
    {
      flush_word();
      tokens.emplace_back(TokenType::LESS_THAN, "<");
      cursor++;
    }
    else if (current_char == '>')
    {
      flush_word();
      tokens.emplace_back(TokenType::GREATER_THAN, ">");
      cursor++;
    }
    else if (current_char == '+')
    {
      flush_word();
      tokens.emplace_back(TokenType::ADD, "+");
      cursor++;
    }
    else if (current_char == '*')
    {
      flush_word();
      tokens.emplace_back(TokenType::MUL, "*");
      cursor++;
    }
    else if (current_char == '/')
    {
      flush_word();
      tokens.emplace_back(TokenType::DIV, "/");
      cursor++;
    }
    else if (current_char == '%')
    {
      flush_word();
      tokens.emplace_back(TokenType::MOD, "%");
      cursor++;
    }
    else if (current_char == '&')
    {
      flush_word();
      tokens.emplace_back(TokenType::BIT_AND, "&");
      cursor++;
    }
    else if (current_char == '|')
    {
      flush_word();
      tokens.emplace_back(TokenType::BIT_OR, "|");
      cursor++;
    }
    else if (current_char == '(')
    {
      flush_word();
      tokens.emplace_back(TokenType::LPAREN, "(");
      cursor++;
    }
    else if (current_char == ')')
    {
      flush_word();
      tokens.emplace_back(TokenType::RPAREN, ")");
      cursor++;
    }

    else if (current_char == '-')
    {
      flush_word();
      tokens.emplace_back(TokenType::SUB, "-");
      cursor++;
    }
    else
    {
      current_word += current_char;
      cursor++;
    }
  }

  flush_word();
  return tokens;
}

char Liner::peek_next() const
{
  if (cursor + 1 < source.length())
  {
    return source[cursor + 1];
  }
  return '\0';
}

void Liner::consume_string()
{
  cursor++;
  std::string literal = "";
  bool closed = false;

  while (cursor < source.length())
  {
    if (source[cursor] == '"')
    {
      closed = true;
      break;
    }
    literal += source[cursor];
    cursor++;
  }

  if (closed)
  {
    tokens.emplace_back(TokenType::STRING, literal);
    cursor++;
  }
  else
  {

    tokens.emplace_back(TokenType::UNKNOWN, "\"" + literal);
  }
}

void Liner::flush_word()
{
  if (current_word.empty())
  {
    return;
  }

  tokens.emplace_back(generate_word_token(current_word));
  current_word.clear();
}

Token Liner::generate_word_token(const std::string &word) const
{
  std::string lower = word;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

  auto it = keywords.find(lower);
  if (it != keywords.end())
  {
    return Token(it->second, word);
  }

  if (word.size() > 2 && word[0] == '0' && (word[1] == 'x' || word[1] == 'X'))
  {
    bool is_hex = std::all_of(word.begin() + 2, word.end(), [](unsigned char c)
                              { return std::isxdigit(c); });
    if (is_hex)
      return Token(TokenType::HEX_NUMBER, word);
  }

  if (std::all_of(word.begin(), word.end(), ::isdigit))
  {
    return Token(TokenType::NUMBER, word);
  }

  if (!word.empty() && (std::isalpha(word[0]) || word[0] == '_'))
  {
    bool validId = std::all_of(word.begin() + 1, word.end(), [](unsigned char c)
                               { return std::isalnum(c) || c == '_'; });

    if (validId)
    {
      return Token(TokenType::IDENTIFIER, word);
    }
  }
  return Token(TokenType::UNKNOWN, word);
}
