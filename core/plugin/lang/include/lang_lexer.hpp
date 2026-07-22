#pragma once

#include <string>
#include <vector>

#include "lang_token.hpp"

namespace lang
{

// Tokenizes a whole .str source file. Emits one Eol token per line break,
// a final Eof token, and skips '#' comments. Keywords are case-insensitive.
std::vector<Token> lex(const std::string &source);

} // namespace lang
