#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include "colors.hpp"
#include "lang_token.hpp"

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

inline void check(TestRunner& tr,
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

inline void check_token(TestRunner& tr,
                        const lang::Token &token,
                        lang::TokKind kind,
                        const std::string &text,
                        const std::string &msg)
{
  check(tr, token.kind == kind,
        msg + " (kind) | expected: " + lang::tok_kind_name(kind) +
            " got: " + lang::tok_kind_name(token.kind) +
            " ('" + token.text + "')");
  check(tr, token.text, text, msg + " (text)");
}
