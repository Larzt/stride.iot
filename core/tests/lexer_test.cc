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

  std::string line = "device=led name=mL pin=17\n\
device=button name=mB pin=35\n\
loop -1\n\
if mB == 1\n\
write=mL on\n\
else\n\
write=mL off\n\
endif\n\
dloop";

  auto tokens = tokenize(line);
  check(runner, tokens.size() > 0, true, "non empty");
  check_token(runner, tokens[0], TokenType::DEVICE, "device", "first token");
  check_token(runner, tokens[2], TokenType::LED, "led", "led type");
  check_token(runner, tokens[18], TokenType::LOOP, "loop", "loop keyword");
  check_token(runner, tokens[20], TokenType::IF, "if", "if keyword");
  check_token(runner, tokens[22], TokenType::IS_EQUAL, "==", "equal operator");
  check_token(runner, tokens[24], TokenType::WRITE, "write", "write command");
  check_token(runner, tokens[33], TokenType::ENDIF, "endif", "endif keyword");
  check_token(runner, tokens.back(), TokenType::DLOOP, "dloop", "dloop at the end");
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
