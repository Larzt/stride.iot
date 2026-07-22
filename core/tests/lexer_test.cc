#include <string>
#include <vector>

#include "lang_lexer.hpp"

#include "check.hpp"

using lang::lex;
using lang::TokKind;

static void testKeywordsCaseInsensitive(TestRunner &runner)
{
  runner.setTest("Lexer: keywords case-insensitive");

  auto tokens = lex("REPEAT End wHen TURN toggle");

  check_token(runner, tokens[0], TokKind::KwRepeat, "REPEAT", "repeat");
  check_token(runner, tokens[1], TokKind::KwEnd, "End", "end");
  check_token(runner, tokens[2], TokKind::KwWhen, "wHen", "when");
  check_token(runner, tokens[3], TokKind::KwTurn, "TURN", "turn");
  check_token(runner, tokens[4], TokKind::KwToggle, "toggle", "toggle");
}

static void testAllStatementKeywords(TestRunner &runner)
{
  runner.setTest("Lexer: statement keywords");

  auto tokens = lex("led button buzzer pin set wait print show log stop "
                    "if else end repeat forever while until when every i2c");

  TokKind expected[] = {
      TokKind::KwLed, TokKind::KwButton, TokKind::KwBuzzer, TokKind::KwPin,
      TokKind::KwSet, TokKind::KwWait, TokKind::KwPrint, TokKind::KwShow,
      TokKind::KwLog, TokKind::KwStop, TokKind::KwIf, TokKind::KwElse,
      TokKind::KwEnd, TokKind::KwRepeat, TokKind::KwForever, TokKind::KwWhile,
      TokKind::KwUntil, TokKind::KwWhen, TokKind::KwEvery, TokKind::KwI2c};

  for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++)
    check(runner, tokens[i].kind == expected[i],
          std::string("keyword #") + std::to_string(i) + " (" +
              tokens[i].text + ")");
}

static void testExpressionKeywords(TestRunner &runner)
{
  runner.setTest("Lexer: expression keywords");

  auto tokens = lex("on off and or not is pressed released signed16");

  check(runner, tokens[0].kind == TokKind::KwOn, "on");
  check(runner, tokens[1].kind == TokKind::KwOff, "off");
  check(runner, tokens[2].kind == TokKind::KwAnd, "and");
  check(runner, tokens[3].kind == TokKind::KwOr, "or");
  check(runner, tokens[4].kind == TokKind::KwNot, "not");
  check(runner, tokens[5].kind == TokKind::KwIs, "is");
  check(runner, tokens[6].kind == TokKind::KwPressed, "pressed");
  check(runner, tokens[7].kind == TokKind::KwReleased, "released");
  check(runner, tokens[8].kind == TokKind::KwSigned16, "signed16");
}

static void testContextualWordsAreNames(TestRunner &runner)
{
  runner.setTest("Lexer: contextual words are identifiers");

  // These must stay usable as variable names: the parser gives them meaning
  // only in their statement context.
  auto tokens = lex("to times register value size into expander ms s min "
                    "write read little endian");

  for (size_t i = 0; i < 14; i++)
    check(runner, tokens[i].kind == TokKind::Name,
          "contextual word '" + tokens[i].text + "' is a Name");
}

static void testIdentifiers(TestRunner &runner)
{
  runner.setTest("Lexer: identifiers");

  auto tokens = lex("myLed temp sensor_1 _hidden");

  check_token(runner, tokens[0], TokKind::Name, "myLed", "identifier 1");
  check_token(runner, tokens[1], TokKind::Name, "temp", "identifier 2");
  check_token(runner, tokens[2], TokKind::Name, "sensor_1", "identifier 3");
  check_token(runner, tokens[3], TokKind::Name, "_hidden", "identifier 4");
}

static void testNumbers(TestRunner &runner)
{
  runner.setTest("Lexer: numbers");

  auto tokens = lex("123 0xFF 0X1b 42");

  check_token(runner, tokens[0], TokKind::Number, "123", "decimal");
  check(runner, tokens[0].num, 123, "decimal value");
  check_token(runner, tokens[1], TokKind::Number, "0xFF", "hex");
  check(runner, tokens[1].num, 255, "hex value");
  check_token(runner, tokens[2], TokKind::Number, "0X1b", "hex mixed case");
  check(runner, tokens[2].num, 27, "hex mixed case value");
  check_token(runner, tokens[3], TokKind::Number, "42", "decimal 2");
}

static void testDecimalLiteral(TestRunner &runner)
{
  runner.setTest("Lexer: decimal literal");

  auto tokens = lex("1.5 0.25");

  check_token(runner, tokens[0], TokKind::Decimal, "1.5", "1.5");
  check_token(runner, tokens[1], TokKind::Decimal, "0.25", "0.25");
}

static void testGluedUnitIsUnknown(TestRunner &runner)
{
  runner.setTest("Lexer: number glued to letters");

  auto tokens = lex("5s 12abc");

  check_token(runner, tokens[0], TokKind::Unknown, "5s", "5s");
  check_token(runner, tokens[1], TokKind::Unknown, "12abc", "12abc");
}

static void testComments(TestRunner &runner)
{
  runner.setTest("Lexer: comments");

  auto tokens = lex("turn light on # esto es un comentario\n# linea entera");

  check(runner, tokens[0].kind == TokKind::KwTurn, "turn");
  check(runner, tokens[1].kind == TokKind::Name, "light");
  check(runner, tokens[2].kind == TokKind::KwOn, "on");
  check(runner, tokens[3].kind == TokKind::Eol, "eol after comment");
  check(runner, tokens[4].kind == TokKind::Eol, "eol of comment-only line");
  check(runner, tokens[5].kind == TokKind::Eof, "eof");
}

static void testLineNumbers(TestRunner &runner)
{
  runner.setTest("Lexer: line numbers");

  auto tokens = lex("a = 1\n\nb = 2");

  check(runner, tokens[0].line == 1, "a on line 1");
  check(runner, tokens[0].text, std::string("a"), "first token is a");
  // tokens: a = 1 EOL EOL b = 2 EOL EOF
  check(runner, tokens[5].text, std::string("b"), "token 5 is b");
  check(runner, tokens[5].line == 3, "b on line 3");
}

static void testStrings(TestRunner &runner)
{
  runner.setTest("Lexer: strings");

  auto tokens = lex("print \"hola mundo\" \"\"");

  check_token(runner, tokens[1], TokKind::String, "hola mundo", "string");
  check_token(runner, tokens[2], TokKind::String, "", "empty string");
}

static void testUnclosedString(TestRunner &runner)
{
  runner.setTest("Lexer: unclosed string");

  auto tokens = lex("print \"sin cerrar");

  check(runner, tokens[1].kind == TokKind::Unknown, "unclosed is Unknown");
}

static void testArrowIsUnknown(TestRunner &runner)
{
  runner.setTest("Lexer: old arrow operator");

  auto tokens = lex("0 -> counter");

  check_token(runner, tokens[1], TokKind::Unknown, "->", "arrow is Unknown");
}

static void testOperators(TestRunner &runner)
{
  runner.setTest("Lexer: operators");

  auto tokens = lex("= + - * / % << >> & | == != < <= > >= ( )");

  TokKind expected[] = {
      TokKind::Assign, TokKind::Plus, TokKind::Minus, TokKind::Star,
      TokKind::Slash, TokKind::Percent, TokKind::Shl, TokKind::Shr,
      TokKind::Amp, TokKind::Pipe, TokKind::Eq, TokKind::Neq,
      TokKind::Lt, TokKind::Le, TokKind::Gt, TokKind::Ge,
      TokKind::LParen, TokKind::RParen};

  for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++)
    check(runner, tokens[i].kind == expected[i],
          std::string("operator #") + std::to_string(i) + " (" +
              tokens[i].text + ")");
}

static void testShiftNotConfusedWithComparison(TestRunner &runner)
{
  runner.setTest("Lexer: shift vs comparison");

  auto tokens = lex("a << 1 b <= 1 c >> 1 d >= 1");

  check(runner, tokens[1].kind == TokKind::Shl, "<<");
  check(runner, tokens[4].kind == TokKind::Le, "<=");
  check(runner, tokens[7].kind == TokKind::Shr, ">>");
  check(runner, tokens[10].kind == TokKind::Ge, ">=");
}

static void testEmptyInput(TestRunner &runner)
{
  runner.setTest("Lexer: empty input");

  auto tokens = lex("");

  check(runner, tokens.size(), static_cast<size_t>(2), "only Eol + Eof");
  check(runner, tokens[0].kind == TokKind::Eol, "eol");
  check(runner, tokens[1].kind == TokKind::Eof, "eof");
}

static void testGarbage(TestRunner &runner)
{
  runner.setTest("Lexer: garbage characters");

  auto tokens = lex("@ $ ?");

  check(runner, tokens[0].kind == TokKind::Unknown, "@");
  check(runner, tokens[1].kind == TokKind::Unknown, "$");
  check(runner, tokens[2].kind == TokKind::Unknown, "?");
}

void run_lexer_tests(TestRunner &runner)
{
  testKeywordsCaseInsensitive(runner);
  testAllStatementKeywords(runner);
  testExpressionKeywords(runner);
  testContextualWordsAreNames(runner);
  testIdentifiers(runner);
  testNumbers(runner);
  testDecimalLiteral(runner);
  testGluedUnitIsUnknown(runner);
  testComments(runner);
  testLineNumbers(runner);
  testStrings(runner);
  testUnclosedString(runner);
  testArrowIsUnknown(runner);
  testOperators(runner);
  testShiftNotConfusedWithComparison(runner);
  testEmptyInput(runner);
  testGarbage(runner);
}
