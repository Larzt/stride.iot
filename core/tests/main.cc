#include <iostream>

#include "check.hpp"

void run_lexer_tests(TestRunner &runner);
void run_parser_tests(TestRunner &runner);

int main()
{
  TestRunner runner;

  run_lexer_tests(runner);
  run_parser_tests(runner);

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
