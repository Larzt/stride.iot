#pragma once

#include "lexer.hpp"

#include <string>
#include <vector>

class Liner
{
public:
  explicit Liner(const std::string &line);
  std::vector<Token> process();

private:
  const std::string &source;
  size_t cursor = 0;
  std::string current_word = "";
  std::vector<Token> tokens;

  char peek_next() const;
  void consume_string();
  void flush_word();
  Token generate_word_token(const std::string &word) const;
};
