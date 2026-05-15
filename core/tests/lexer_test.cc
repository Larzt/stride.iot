#include <iostream>
#include <vector>
#include <string>
#include <cassert>

#include "lexer.hpp"
#include "liner.hpp"

#include "check.hpp"

// -----------------------------------------------------
// Happy Path
// -----------------------------------------------------
void testKeywordTokens(TestRunner &runner)
{
  runner.setTest("Test Keywords");

  auto tokens = tokenize("device write read if endif");

  check_token(runner, tokens[0], TokenType::DEVICE, "device", "device");
  check_token(runner, tokens[1], TokenType::WRITE, "write", "write");
  check_token(runner, tokens[2], TokenType::READ, "read", "read");
  check_token(runner, tokens[3], TokenType::IF, "if", "if");
  check_token(runner, tokens[4], TokenType::ENDIF, "endif", "endif");
}

void testIdentifierTokens(TestRunner &runner)
{
  runner.setTest("Test Identifiers");

  auto tokens = tokenize("myLed temp sensor_1");

  check_token(runner, tokens[0], TokenType::IDENTIFIER, "myLed", "identifier 1");
  check_token(runner, tokens[1], TokenType::IDENTIFIER, "temp", "identifier 2");
  check_token(runner, tokens[2], TokenType::IDENTIFIER, "sensor_1", "identifier 3");
}

void testNumberTokens(TestRunner &runner)
{
  runner.setTest("Test Numbers");

  auto tokens = tokenize("123 0xFF 42");

  check_token(runner, tokens[0], TokenType::NUMBER, "123", "number 1");
  check_token(runner, tokens[1], TokenType::HEX_NUMBER, "0xFF", "hex number");
  check_token(runner, tokens[2], TokenType::NUMBER, "42", "number 2");
}

void testAssignmentTokens(TestRunner &runner)
{
  runner.setTest("Test Assignment");

  auto tokens = tokenize("a = 5  3 -> b");

  check_token(runner, tokens[0], TokenType::IDENTIFIER, "a", "identifier");
  check_token(runner, tokens[1], TokenType::ASSIGN, "=", "assign");
  check_token(runner, tokens[2], TokenType::NUMBER, "5", "value");

  check_token(runner, tokens[3], TokenType::NUMBER, "3", "value 2");
  check_token(runner, tokens[4], TokenType::ARROW, "->", "arrow");
  check_token(runner, tokens[5], TokenType::IDENTIFIER, "b", "target");
}

void testComparisonTokens(TestRunner &runner)
{
  runner.setTest("Test Comparisons");

  auto tokens = tokenize("a == b a != b a < b a <= b a > b a >= b");

  check_token(runner, tokens[1], TokenType::IS_EQUAL, "==", "==");
  check_token(runner, tokens[4], TokenType::NOT_EQUAL, "!=", "!=");
  check_token(runner, tokens[7], TokenType::LESS_THAN, "<", "<");
  check_token(runner, tokens[10], TokenType::LESS_EQUAL, "<=", "<=");
  check_token(runner, tokens[13], TokenType::GREATER_THAN, ">", ">");
  check_token(runner, tokens[16], TokenType::GREATER_EQUAL, ">=", ">=");
}

void testValueTokens(TestRunner &runner)
{
  runner.setTest("Test Values");

  auto tokens = tokenize("on OFF");

  check_token(runner, tokens[0], TokenType::VALUE, "on", "value on");
  check_token(runner, tokens[1], TokenType::VALUE, "OFF", "value off");
}

void testPrintTokens(TestRunner &runner)
{
  runner.setTest("Test Print");

  auto tokens = tokenize("print \"hola mundo\"");

  check_token(runner, tokens[0], TokenType::PRINT, "print", "print");
  check_token(runner, tokens[1], TokenType::STRING, "hola mundo", "string message");
}

void testI2CTokens(TestRunner &runner)
{
  runner.setTest("Test I2C");

  auto tokens = tokenize("i2c read 0x76 0xFA 3 -> temp");

  check_token(runner, tokens[0], TokenType::I2C, "i2c", "i2c");
  check_token(runner, tokens[1], TokenType::READ, "read", "read");
  check_token(runner, tokens[2], TokenType::HEX_NUMBER, "0x76", "addr");
  check_token(runner, tokens[3], TokenType::HEX_NUMBER, "0xFA", "register");
  check_token(runner, tokens[4], TokenType::NUMBER, "3", "length");
  check_token(runner, tokens[5], TokenType::ARROW, "->", "arrow");
  check_token(runner, tokens[6], TokenType::IDENTIFIER, "temp", "target");
}

void testDeviceLedDeclaration(TestRunner &runner)
{
  runner.setTest("Device declaration");

  auto tokens = tokenize("device=led name=myLed pin=16");

  check_token(runner, tokens[0], TokenType::DEVICE, "device", "device");
  check_token(runner, tokens[2], TokenType::LED, "led", "led");
  check_token(runner, tokens[3], TokenType::NAME, "name", "name");
  check_token(runner, tokens[5], TokenType::IDENTIFIER, "myLed", "identifier");

  check_token(runner, tokens[6], TokenType::PIN, "pin", "pin");
  check_token(runner, tokens[8], TokenType::NUMBER, "16", "number");
}

void testDeviceBuzzerDeclaration(TestRunner &runner)
{
  runner.setTest("Device declaration");

  auto tokens = tokenize("device=buzzer name=mybuz pin=14");

  check_token(runner, tokens[0], TokenType::DEVICE, "device", "device");
  check_token(runner, tokens[2], TokenType::BUZZER, "buzzer", "buzzer");
  check_token(runner, tokens[3], TokenType::NAME, "name", "name");
  check_token(runner, tokens[5], TokenType::IDENTIFIER, "mybuz", "identifier");

  check_token(runner, tokens[6], TokenType::PIN, "pin", "pin");
  check_token(runner, tokens[8], TokenType::NUMBER, "14", "number");
}

void testDeviceButtonDeclaration(TestRunner &runner)
{
  runner.setTest("Device declaration");

  auto tokens = tokenize("device=button name=myButton pin=16");

  check_token(runner, tokens[0], TokenType::DEVICE, "device", "device");
  check_token(runner, tokens[2], TokenType::BUTTON, "button", "button");
  check_token(runner, tokens[3], TokenType::NAME, "name", "name");
  check_token(runner, tokens[5], TokenType::IDENTIFIER, "myButton", "identifier");

  check_token(runner, tokens[6], TokenType::PIN, "pin", "pin");
  check_token(runner, tokens[8], TokenType::NUMBER, "16", "number");
}

void testProgramScenarios(TestRunner &runner)
{
  runner.setTest("Test Program Scenarios");

  // Tokenizar línea por línea como hace el intérprete real
  auto t0 = tokenize("device=led name=mL pin=17");
  auto t1 = tokenize("device=button name=mB pin=35");
  auto t2 = tokenize("loop -1");
  auto t3 = tokenize("if mB == 1");
  auto t4 = tokenize("write=mL on");
  auto t5 = tokenize("else");
  auto t6 = tokenize("write=mL off");
  auto t7 = tokenize("endif");
  auto t8 = tokenize("dloop");

  check_token(runner, t0[0], TokenType::DEVICE, "device", "device keyword");
  check_token(runner, t0[2], TokenType::LED, "led", "led type");
  check_token(runner, t2[0], TokenType::LOOP, "loop", "loop keyword");
  check_token(runner, t3[0], TokenType::IF, "if", "if keyword");
  check_token(runner, t3[2], TokenType::IS_EQUAL, "==", "equal operator");
  check_token(runner, t4[0], TokenType::WRITE, "write", "write command");
  check_token(runner, t7[0], TokenType::ENDIF, "endif", "endif keyword");
  check_token(runner, t8[0], TokenType::DLOOP, "dloop", "dloop at the end");
}
void testParenthesesTokens(TestRunner &runner)
{
  runner.setTest("Test Parentheses");

  auto tokens = tokenize("result = (a + b) * c");

  check(runner, tokens.size(), static_cast<size_t>(9), "exactamente 9 tokens");
  check_token(runner, tokens[2], TokenType::LPAREN, "(", "lparen");
  check_token(runner, tokens[6], TokenType::RPAREN, ")", "rparen");
  check_token(runner, tokens[7], TokenType::MUL,    "*", "mul after rparen");
}

void testI2CReadLETokens(TestRunner &runner)
{
  runner.setTest("Test I2C READLE");

  auto tokens = tokenize("i2c readle 0x76 0x88 2 -> dig_T1");

  check(runner, tokens.size(), static_cast<size_t>(7), "exactamente 7 tokens");
  check_token(runner, tokens[0], TokenType::I2C,        "i2c",    "i2c");
  check_token(runner, tokens[1], TokenType::READLE,     "readle", "readle");
  check_token(runner, tokens[2], TokenType::HEX_NUMBER, "0x76",   "addr");
  check_token(runner, tokens[3], TokenType::HEX_NUMBER, "0x88",   "register");
  check_token(runner, tokens[4], TokenType::NUMBER,     "2",      "bytes");
  check_token(runner, tokens[5], TokenType::ARROW,      "->",     "arrow");
  check_token(runner, tokens[6], TokenType::IDENTIFIER, "dig_T1", "target");
}

void testSign16Tokens(TestRunner &runner)
{
  runner.setTest("Test SIGN16");

  auto tokens = tokenize("sign16 dig_T2");

  check(runner, tokens.size(), static_cast<size_t>(2), "exactamente 2 tokens");
  check_token(runner, tokens[0], TokenType::SIGN16,     "sign16", "sign16 keyword");
  check_token(runner, tokens[1], TokenType::IDENTIFIER, "dig_T2", "variable");
}

void testSign16UpperCase(TestRunner &runner)
{
  runner.setTest("Test SIGN16 uppercase");

  auto tokens = tokenize("SIGN16 myVar");

  check(runner, tokens.size(), static_cast<size_t>(2), "exactamente 2 tokens");
  check_token(runner, tokens[0], TokenType::SIGN16,     "SIGN16", "sign16 uppercase");
  check_token(runner, tokens[1], TokenType::IDENTIFIER, "myVar",  "variable");
}

// -----------------------------------------------------
// Error path
// -----------------------------------------------------
void testInvalidTokens(TestRunner &runner)
{
  runner.setTest("Test Invalid Tokens");

  auto tokens = tokenize("@ @@ ###");

  check(runner, tokens.size() > 0, true, "tokens generated");

  for (const auto &t : tokens)
  {
    check(runner, t.type, TokenType::UNKNOWN, "invalid token detected");
  }
}

void testInvalidHex(TestRunner &runner)
{
  runner.setTest("Test Invalid Hex");

  auto tokens = tokenize("0x 0xG1 0xZZ");
  check(runner, tokens.size() > 0, true, "tokens exist");
  for (const auto &t : tokens)
  {
    check(runner, t.type == TokenType::HEX_NUMBER, false, "invalid hex should not be valid");
  }
}

void testUnclosedString(TestRunner &runner)
{
  runner.setTest("Test Unclosed String");

  auto tokens = tokenize("print \"hola");
  check(runner, tokens.size() > 0, true, "tokens exist");
  check(runner, tokens[1].type == TokenType::STRING, false, "string should be invalid");
}

void testInvalidOperators(TestRunner &runner)
{
  runner.setTest("Test Invalid Operators");

  auto tokens = tokenize("a === b a <> b a => b");
  for (const auto &t : tokens)
  {
    check(runner, t.type == TokenType::IS_EQUAL, false, "invalid operator should fail");
  }
}

void testBrokenAssignments(TestRunner &runner)
{
  runner.setTest("Test Broken Assignments");
  auto tokens = tokenize("= 5");

  check(runner, tokens[0].type, TokenType::ASSIGN, "Token = detectado");
  check(runner, tokens[1].type, TokenType::NUMBER, "Token 5 detectado");

  bool isValidSequence = (tokens.size() >= 3);
  check(runner, isValidSequence, false, "La secuencia es demasiado corta para ser una asignación");
}

void testInvalidKeywords(TestRunner &runner)
{
  runner.setTest("Test Invalid Keywords");
  auto tokens = tokenize("outpu writ reaad iff endiff");
  for (const auto &t : tokens)
  {
    check(runner, t.type == TokenType::DEVICE, false, "typo keyword");
    check(runner, t.type == TokenType::WRITE, false, "typo keyword");
  }
}

void testWeirdSpacing(TestRunner &runner)
{
  runner.setTest("Test Weird Spacing");
  auto tokens = tokenize("a=5    b   =    6");
  check(runner, tokens.size() > 0, true, "tokens exist");
  check_token(runner, tokens[0], TokenType::IDENTIFIER, "a", "a");
  check_token(runner, tokens[1], TokenType::ASSIGN, "=", "=");
  check_token(runner, tokens[2], TokenType::NUMBER, "5", "5");
}

void testEmptyInput(TestRunner &runner)
{
  runner.setTest("Test Empty Input");
  auto tokens = tokenize("");
  check(runner, tokens.size(), static_cast<size_t>(0), "empty input");
}

void testOnlyGarbage(TestRunner &runner)
{
  runner.setTest("Test Only Garbage");
  auto tokens = tokenize("$$$$$");
  for (const auto &t : tokens)
  {
    check(runner, t.type, TokenType::UNKNOWN, "garbage token");
  }
}

void testArithmeticOperators(TestRunner &runner)
{
  runner.setTest("Test Arithmetic Operators");

  auto tokens = tokenize("a + b a - b a * b a / b a % b");

  check_token(runner, tokens[1], TokenType::ADD, "+", "+");
  check_token(runner, tokens[4], TokenType::SUB, "-", "-");
  check_token(runner, tokens[7], TokenType::MUL, "*", "*");
  check_token(runner, tokens[10], TokenType::DIV, "/", "/");
  check_token(runner, tokens[13], TokenType::MOD, "%", "%");
}

void testBitwiseOperators(TestRunner &runner)
{
  runner.setTest("Test Bitwise Operators");

  auto tokens = tokenize("a & b a | b a << b a >> b");

  check_token(runner, tokens[1], TokenType::BIT_AND, "&", "&");
  check_token(runner, tokens[4], TokenType::BIT_OR, "|", "|");
  check_token(runner, tokens[7], TokenType::SHL, "<<", "<<");
  check_token(runner, tokens[10], TokenType::SHR, ">>", ">>");
}

void testArrowNotConfusedWithSub(TestRunner &runner)
{
  runner.setTest("Test Arrow Not Confused With Sub");

  // '->' debe ser ARROW, no SUB + GREATER_THAN
  auto tokens = tokenize("3 -> b");

  check(runner, tokens.size(), static_cast<size_t>(3), "exactamente 3 tokens");
  check_token(runner, tokens[0], TokenType::NUMBER, "3", "numero");
  check_token(runner, tokens[1], TokenType::ARROW, "->", "arrow");
  check_token(runner, tokens[2], TokenType::IDENTIFIER, "b", "variable");
}

void testShrNotConfusedWithGreaterEqual(TestRunner &runner)
{
  runner.setTest("Test SHR Not Confused With >= or >");

  // '>>' debe ser SHR, no dos GREATER_THAN
  auto tokens = tokenize("a >> 4");

  check(runner, tokens.size(), static_cast<size_t>(3), "exactamente 3 tokens");
  check_token(runner, tokens[0], TokenType::IDENTIFIER, "a", "variable");
  check_token(runner, tokens[1], TokenType::SHR, ">>", "shr");
  check_token(runner, tokens[2], TokenType::NUMBER, "4", "numero");
}

void testShlNotConfusedWithLessEqual(TestRunner &runner)
{
  runner.setTest("Test SHL Not Confused With <= or <");

  // '<<' debe ser SHL, no dos LESS_THAN
  auto tokens = tokenize("a << 4");

  check(runner, tokens.size(), static_cast<size_t>(3), "exactamente 3 tokens");
  check_token(runner, tokens[0], TokenType::IDENTIFIER, "a", "variable");
  check_token(runner, tokens[1], TokenType::SHL, "<<", "shl");
  check_token(runner, tokens[2], TokenType::NUMBER, "4", "numero");
}

void testExpressionAssignment(TestRunner &runner)
{
  runner.setTest("Test Expression Assignment");

  // var = a + b
  auto tokens = tokenize("result = a + b");

  check(runner, tokens.size(), static_cast<size_t>(5), "exactamente 5 tokens");
  check_token(runner, tokens[0], TokenType::IDENTIFIER, "result", "variable destino");
  check_token(runner, tokens[1], TokenType::ASSIGN, "=", "assign");
  check_token(runner, tokens[2], TokenType::IDENTIFIER, "a", "operando izq");
  check_token(runner, tokens[3], TokenType::ADD, "+", "operador");
  check_token(runner, tokens[4], TokenType::IDENTIFIER, "b", "operando der");
}

void testExpressionWithHex(TestRunner &runner)
{
  runner.setTest("Test Expression With Hex");

  // caso real del BMP280: temp_raw = t_msb << 12
  auto tokens = tokenize("temp_raw = t_msb << 12");

  check(runner, tokens.size(), static_cast<size_t>(5), "exactamente 5 tokens");
  check_token(runner, tokens[0], TokenType::IDENTIFIER, "temp_raw", "variable destino");
  check_token(runner, tokens[1], TokenType::ASSIGN, "=", "assign");
  check_token(runner, tokens[2], TokenType::IDENTIFIER, "t_msb", "operando");
  check_token(runner, tokens[3], TokenType::SHL, "<<", "shift left");
  check_token(runner, tokens[4], TokenType::NUMBER, "12", "bits");
}

void testExpressionWithBitOr(TestRunner &runner)
{
  runner.setTest("Test Expression With Bit OR");

  // caso real del BMP280: temp_raw = temp_raw | t_lsb_s
  auto tokens = tokenize("temp_raw = temp_raw | t_lsb_s");

  check(runner, tokens.size(), static_cast<size_t>(5), "exactamente 5 tokens");
  check_token(runner, tokens[3], TokenType::BIT_OR, "|", "bit or");
}

void testNegativeNumberInExpression(TestRunner &runner)
{
  runner.setTest("Test Negative Number In Expression");

  // '-' entre dos valores debe ser SUB, no parte de un número negativo
  auto tokens = tokenize("a = b - 1");

  check(runner, tokens.size(), static_cast<size_t>(5), "exactamente 5 tokens");
  check_token(runner, tokens[3], TokenType::SUB, "-", "sub operator");
  check_token(runner, tokens[4], TokenType::NUMBER, "1", "numero");
}

int main()
{
  TestRunner runner;

  // Happy Path
  testKeywordTokens(runner);
  testIdentifierTokens(runner);
  testNumberTokens(runner);
  testAssignmentTokens(runner);
  testComparisonTokens(runner);
  testPrintTokens(runner);
  testI2CTokens(runner);
  testDeviceLedDeclaration(runner);
  testDeviceBuzzerDeclaration(runner);
  testDeviceButtonDeclaration(runner);
  testProgramScenarios(runner);

  // Error Path
  testUnclosedString(runner);
  testInvalidTokens(runner);
  testInvalidHex(runner);
  testInvalidOperators(runner);
  testBrokenAssignments(runner);
  testInvalidKeywords(runner);
  testWeirdSpacing(runner);
  testEmptyInput(runner);
  testOnlyGarbage(runner);

  // Operadores aritméticos y bitwise
  testArithmeticOperators(runner);
  testBitwiseOperators(runner);
  testArrowNotConfusedWithSub(runner);
  testShrNotConfusedWithGreaterEqual(runner);
  testShlNotConfusedWithLessEqual(runner);
  testExpressionAssignment(runner);
  testExpressionWithHex(runner);
  testExpressionWithBitOr(runner);
  testNegativeNumberInExpression(runner);
  testParenthesesTokens(runner);
  testI2CReadLETokens(runner);
  testSign16Tokens(runner);
  testSign16UpperCase(runner);

  std::cout << CYAN << "PASSED: " << runner.passed << std::endl;
  std::cout << CYAN << "FAILED: " << runner.failed << std::endl;

  if (!runner.failedMessages.empty())
  {
    std::cout << std::endl
              << RED << "FAILED TESTS:" << RESET << std::endl;
    for (const auto &msg : runner.failedMessages)
    {
      std::cout << RED << msg << RESET << std::endl;
    }
  }

  return runner.allPassed() ? 0 : 1;
}
