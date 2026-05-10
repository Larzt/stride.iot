#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include "colors.hpp"

struct TestRunner
{
  int passed = 0;
  int failed = 0;
  std::vector<std::string> failedMessages;
  std::string currentTest;

  TestRunner(const std::string& testName = "")
  {
    currentTest = testName;
  }

  void setTest(const std::string& name)
  {
    currentTest = name;
  }

  void pass()
  {
    passed++;
  }

  void fail(const std::string& msg)
  {
    failed++;
    failedMessages.push_back("[" + currentTest + "] " + msg);
  }

  bool allPassed() const
  {
    return failed == 0;
  }
};

template <typename T>
std::string toString(const T &value)
{
  std::ostringstream oss;
  oss << value;
  return oss.str();
}

template <typename T>
void check(TestRunner& tr,
           const T& actual,
           const T& expected,
           const std::string& msg)
{
  if (actual != expected)
  {
    tr.fail(msg + " | expected: " + toString(expected) +
            " got: " + toString(actual));
  }
  else
  {
    tr.pass();
  }
}

void check(TestRunner& tr,
           bool condition,
           const std::string& msg)
{
  if (!condition)
  {
    tr.fail(msg);
  }
  else
  {
    tr.pass();
  }
}

void check_token(TestRunner& tr,
                const Token &token,
                TokenType type,
                const std::string &value,
                const std::string &msg)
{
  check(tr, token.type, type, msg + " (type)");
  check(tr, token.value, value, msg + " (value)");
}
