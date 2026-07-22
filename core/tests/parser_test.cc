#include <string>

#include "lang_parser.hpp"

#include "check.hpp"

using lang::DeviceType;
using lang::Expr;
using lang::parse;
using lang::RepeatMode;
using lang::Stmt;
using lang::TokKind;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static bool is_number(const Expr *e, int32_t value)
{
  return e && e->kind == Expr::Kind::Number && e->number == value;
}

static bool is_name(const Expr *e, const std::string &name)
{
  return e && e->kind == Expr::Kind::Name && e->text == name;
}

static bool is_binary(const Expr *e, TokKind op)
{
  return e && e->kind == Expr::Kind::Binary && e->op == op;
}

// ---------------------------------------------------------------------------
// Declarations and simple statements
// ---------------------------------------------------------------------------

static void testDeviceDeclarations(TestRunner &runner)
{
  runner.setTest("Parser: device declarations");

  auto result = parse("led light on pin 17\n"
                      "button btn on pin 32\n"
                      "buzzer horn on pin 26\n"
                      "pin exLed on expander 3\n");

  check(runner, result.ok(), "no errors");
  check(runner, result.program.top.size(), static_cast<size_t>(4),
        "four statements");

  const auto &top = result.program.top;
  check(runner, top[0].kind == Stmt::Kind::DeclareDevice, "led stmt");
  check(runner, top[0].flags == static_cast<uint8_t>(DeviceType::Led),
        "led type");
  check(runner, top[0].name, std::string("light"), "led name");
  check(runner, top[0].value, static_cast<uint32_t>(17), "led pin");

  check(runner, top[1].flags == static_cast<uint8_t>(DeviceType::Button),
        "button type");
  check(runner, top[2].flags == static_cast<uint8_t>(DeviceType::Buzzer),
        "buzzer type");

  check(runner, top[3].flags == static_cast<uint8_t>(DeviceType::ExpanderPin),
        "expander type");
  check(runner, top[3].name, std::string("exLed"), "expander name");
  check(runner, top[3].value, static_cast<uint32_t>(3), "expander pin");
}

static void testTurnToggle(TestRunner &runner)
{
  runner.setTest("Parser: turn / toggle");

  auto result = parse("turn light on\nturn light off\ntoggle light\n");

  check(runner, result.ok(), "no errors");
  const auto &top = result.program.top;
  check(runner, top[0].kind == Stmt::Kind::Turn, "turn stmt");
  check(runner, top[0].flags == 1, "turn on");
  check(runner, top[1].flags == 0, "turn off");
  check(runner, top[2].kind == Stmt::Kind::Toggle, "toggle stmt");
  check(runner, top[2].name, std::string("light"), "toggle name");
}

static void testAssignmentForms(TestRunner &runner)
{
  runner.setTest("Parser: set / = assignment");

  auto result = parse("set count to 5\ncount = count + 1\n");

  check(runner, result.ok(), "no errors");
  const auto &top = result.program.top;

  check(runner, top[0].kind == Stmt::Kind::Set, "set stmt");
  check(runner, top[0].name, std::string("count"), "set name");
  check(runner, is_number(top[0].args[0].get(), 5), "set value");

  check(runner, top[1].kind == Stmt::Kind::Set, "= stmt");
  check(runner, is_binary(top[1].args[0].get(), TokKind::Plus), "= expr");
}

static void testWaitDurations(TestRunner &runner)
{
  runner.setTest("Parser: wait durations");

  auto result = parse("wait 500 ms\n"
                      "wait 1.5 s\n"
                      "wait 2 min\n"
                      "wait delay s\n");

  check(runner, result.ok(), "no errors");
  const auto &top = result.program.top;

  check(runner, top[0].flags == 0, "constant ms");
  check(runner, top[0].value, static_cast<uint32_t>(500), "500 ms");

  check(runner, top[1].value, static_cast<uint32_t>(1500), "1.5 s -> 1500");
  check(runner, top[2].value, static_cast<uint32_t>(120000), "2 min");

  check(runner, top[3].flags == 1, "variable duration keeps expr");
  check(runner, top[3].value, static_cast<uint32_t>(1000), "unit factor s");
  check(runner, is_name(top[3].args[0].get(), "delay"), "magnitude expr");
}

static void testWaitWithoutUnitFails(TestRunner &runner)
{
  runner.setTest("Parser: wait without unit");

  auto result = parse("wait 5\n");

  check(runner, !result.ok(), "error reported");
  check(runner, result.errors[0].line == 1, "error on line 1");
  check(runner,
        result.errors[0].message.find("unidad") != std::string::npos,
        "message mentions the unit");
}

static void testPrintShowLog(TestRunner &runner)
{
  runner.setTest("Parser: print / show / log to");

  auto result = parse("print \"count =\" count + 1\n"
                      "show \"hola\"\n"
                      "log to \"sesion.log\"\n");

  check(runner, result.ok(), "no errors");
  const auto &top = result.program.top;

  check(runner, top[0].kind == Stmt::Kind::Print, "print stmt");
  check(runner, top[0].args.size(), static_cast<size_t>(2), "print args");
  check(runner, top[0].args[0]->kind == Expr::Kind::String, "string arg");
  check(runner, is_binary(top[0].args[1].get(), TokKind::Plus),
        "expression arg is one expression");

  check(runner, top[1].kind == Stmt::Kind::Show, "show stmt");
  check(runner, top[2].kind == Stmt::Kind::LogTo, "log stmt");
  check(runner, top[2].name, std::string("sesion.log"), "log filename");
}

// ---------------------------------------------------------------------------
// Expressions
// ---------------------------------------------------------------------------

static void testArithmeticPrecedence(TestRunner &runner)
{
  runner.setTest("Parser: precedence * over +");

  auto result = parse("x = 1 + 2 * 3\n");

  check(runner, result.ok(), "no errors");
  const Expr *e = result.program.top[0].args[0].get();

  check(runner, is_binary(e, TokKind::Plus), "+ at root");
  check(runner, is_number(e->lhs.get(), 1), "lhs 1");
  check(runner, is_binary(e->rhs.get(), TokKind::Star), "* nested");
  check(runner, is_number(e->rhs->lhs.get(), 2), "2");
  check(runner, is_number(e->rhs->rhs.get(), 3), "3");
}

static void testLogicalPrecedence(TestRunner &runner)
{
  runner.setTest("Parser: precedence and over or");

  auto result = parse("x = a or b and c\n");

  check(runner, result.ok(), "no errors");
  const Expr *e = result.program.top[0].args[0].get();

  check(runner, is_binary(e, TokKind::KwOr), "or at root");
  check(runner, is_name(e->lhs.get(), "a"), "a");
  check(runner, is_binary(e->rhs.get(), TokKind::KwAnd), "and nested");
}

static void testNotBindsLooserThanComparison(TestRunner &runner)
{
  runner.setTest("Parser: not over comparison");

  auto result = parse("x = not a == 1\n");

  check(runner, result.ok(), "no errors");
  const Expr *e = result.program.top[0].args[0].get();

  check(runner, e->kind == Expr::Kind::Unary && e->op == TokKind::KwNot,
        "not at root");
  check(runner, is_binary(e->lhs.get(), TokKind::Eq), "== nested");
}

static void testComparisonOverArithmetic(TestRunner &runner)
{
  runner.setTest("Parser: comparison over arithmetic");

  auto result = parse("x = a < b + 1\n");

  check(runner, result.ok(), "no errors");
  const Expr *e = result.program.top[0].args[0].get();

  check(runner, is_binary(e, TokKind::Lt), "< at root");
  check(runner, is_binary(e->rhs.get(), TokKind::Plus), "+ nested");
}

static void testIsForms(TestRunner &runner)
{
  runner.setTest("Parser: is / is not / is pressed");

  auto result = parse("x = btn is pressed\n"
                      "y = btn is released\n"
                      "z = light is on\n"
                      "w = a is not 3\n");

  check(runner, result.ok(), "no errors");
  const auto &top = result.program.top;

  const Expr *e = top[0].args[0].get();
  check(runner, is_binary(e, TokKind::Eq), "is pressed -> ==");
  check(runner, is_number(e->rhs.get(), 1), "pressed -> 1");

  e = top[1].args[0].get();
  check(runner, is_binary(e, TokKind::Eq), "is released -> ==");
  check(runner, is_number(e->rhs.get(), 0), "released -> 0");

  e = top[2].args[0].get();
  check(runner, is_binary(e, TokKind::Eq), "is on -> ==");
  check(runner, is_number(e->rhs.get(), 1), "on -> 1");

  e = top[3].args[0].get();
  check(runner, is_binary(e, TokKind::Neq), "is not -> !=");
}

static void testSigned16AndUnaryMinus(TestRunner &runner)
{
  runner.setTest("Parser: signed16 and unary minus");

  auto result = parse("x = signed16(raw)\ny = -5 + 3\n");

  check(runner, result.ok(), "no errors");
  const auto &top = result.program.top;

  check(runner, top[0].args[0]->kind == Expr::Kind::Signed16, "signed16");
  check(runner, is_name(top[0].args[0]->lhs.get(), "raw"), "signed16 arg");

  const Expr *e = top[1].args[0].get();
  check(runner, is_binary(e, TokKind::Plus), "+ at root");
  check(runner,
        e->lhs->kind == Expr::Kind::Unary && e->lhs->op == TokKind::Minus,
        "unary minus");
}

static void testParenthesesOverridePrecedence(TestRunner &runner)
{
  runner.setTest("Parser: parentheses");

  auto result = parse("x = (1 + 2) * 3\n");

  check(runner, result.ok(), "no errors");
  const Expr *e = result.program.top[0].args[0].get();

  check(runner, is_binary(e, TokKind::Star), "* at root");
  check(runner, is_binary(e->lhs.get(), TokKind::Plus), "+ nested");
}

// ---------------------------------------------------------------------------
// Blocks and nesting
// ---------------------------------------------------------------------------

static void testIfElseChain(TestRunner &runner)
{
  runner.setTest("Parser: if / else if / else");

  auto result = parse("if a == 1\n"
                      "  turn x on\n"
                      "else if a == 2\n"
                      "  turn x off\n"
                      "else\n"
                      "  toggle x\n"
                      "end\n");

  check(runner, result.ok(), "no errors");
  const Stmt &st = result.program.top[0];

  check(runner, st.kind == Stmt::Kind::If, "if stmt");
  check(runner, st.branches.size(), static_cast<size_t>(3), "three branches");
  check(runner, st.branches[0].first != nullptr, "if has cond");
  check(runner, st.branches[1].first != nullptr, "else if has cond");
  check(runner, st.branches[2].first == nullptr, "else has no cond");
  check(runner, st.branches[2].second.size(), static_cast<size_t>(1),
        "else body");
}

static void testNestedBlocks(TestRunner &runner)
{
  runner.setTest("Parser: nested if inside repeat inside if");

  auto result = parse("if a == 1\n"
                      "  repeat 3 times\n"
                      "    if b == 2\n"
                      "      turn x on\n"
                      "    else\n"
                      "      turn x off\n"
                      "    end\n"
                      "    toggle y\n"
                      "  end\n"
                      "  print \"done\"\n"
                      "end\n");

  check(runner, result.ok(), "no errors");
  check(runner, result.program.top.size(), static_cast<size_t>(1),
        "one top stmt");

  const Stmt &outer_if = result.program.top[0];
  check(runner, outer_if.kind == Stmt::Kind::If, "outer if");
  const auto &outer_body = outer_if.branches[0].second;
  check(runner, outer_body.size(), static_cast<size_t>(2),
        "outer body: repeat + print");

  const Stmt &rep = outer_body[0];
  check(runner, rep.kind == Stmt::Kind::Repeat, "repeat nested");
  check(runner, rep.body.size(), static_cast<size_t>(2),
        "repeat body: if + toggle");

  const Stmt &inner_if = rep.body[0];
  check(runner, inner_if.kind == Stmt::Kind::If, "inner if");
  check(runner, inner_if.branches.size(), static_cast<size_t>(2),
        "inner if has else");
  check(runner, outer_body[1].kind == Stmt::Kind::Print, "print after repeat");
}

static void testRepeatForms(TestRunner &runner)
{
  runner.setTest("Parser: repeat forms");

  auto result = parse("repeat 5 times\nend\n"
                      "repeat 5\nend\n"
                      "repeat forever\nend\n"
                      "repeat while a < 3\nend\n"
                      "repeat until btn is pressed\nend\n");

  check(runner, result.ok(), "no errors");
  const auto &top = result.program.top;

  check(runner, top[0].flags == static_cast<uint8_t>(RepeatMode::Times),
        "times");
  check(runner, top[1].flags == static_cast<uint8_t>(RepeatMode::Times),
        "times without word");
  check(runner, top[2].flags == static_cast<uint8_t>(RepeatMode::Forever),
        "forever");
  check(runner, top[3].flags == static_cast<uint8_t>(RepeatMode::While),
        "while");
  check(runner, top[4].flags == static_cast<uint8_t>(RepeatMode::Until),
        "until");
}

static void testWhenEvery(TestRunner &runner)
{
  runner.setTest("Parser: when / every");

  auto result = parse("when btn pressed\n  toggle light\nend\n"
                      "when btn is released\n  turn light off\nend\n"
                      "every 10 s\n  print \"tick\"\nend\n");

  check(runner, result.ok(), "no errors");
  const auto &top = result.program.top;

  check(runner, top[0].kind == Stmt::Kind::When, "when stmt");
  check(runner, top[0].name, std::string("btn"), "when button");
  check(runner, top[0].flags == 1, "pressed");
  check(runner, top[0].body.size(), static_cast<size_t>(1), "when body");

  check(runner, top[1].flags == 0, "released (with is)");

  check(runner, top[2].kind == Stmt::Kind::Every, "every stmt");
  check(runner, top[2].value, static_cast<uint32_t>(10000), "every 10 s");
  check(runner, top[2].body.size(), static_cast<size_t>(1), "every body");
}

// ---------------------------------------------------------------------------
// I2C
// ---------------------------------------------------------------------------

static void testI2cWrite(TestRunner &runner)
{
  runner.setTest("Parser: i2c write");

  auto result = parse("i2c write 0x76 value 0xF4\n"
                      "i2c write 0x76 register 0xF4 value mode\n");

  check(runner, result.ok(), "no errors");
  const auto &top = result.program.top;

  check(runner, top[0].kind == Stmt::Kind::I2cWrite, "write stmt");
  check(runner, (top[0].flags & 2) == 0, "no register");
  check(runner, top[0].args.size(), static_cast<size_t>(2), "addr + value");

  check(runner, (top[1].flags & 2) != 0, "with register");
  check(runner, top[1].args.size(), static_cast<size_t>(3),
        "addr + reg + value");
  check(runner, is_name(top[1].args[2].get(), "mode"), "value can be a var");
}

static void testI2cRead(TestRunner &runner)
{
  runner.setTest("Parser: i2c read");

  auto result = parse("i2c read 0x76 register 0xFA size 3 into raw\n"
                      "i2c read 0x76 register 0x88 size 2 into cal little\n"
                      "i2c read addr size 1 into x little endian\n");

  check(runner, result.ok(), "no errors");
  const auto &top = result.program.top;

  check(runner, top[0].kind == Stmt::Kind::I2cRead, "read stmt");
  check(runner, top[0].name, std::string("raw"), "into raw");
  check(runner, (top[0].flags & 1) == 0, "big endian default");
  check(runner, (top[0].flags & 2) != 0, "with register");

  check(runner, (top[1].flags & 1) != 0, "little endian");

  check(runner, (top[2].flags & 2) == 0, "no register");
  check(runner, (top[2].flags & 1) != 0, "little endian (with endian word)");
  check(runner, is_name(top[2].args[0].get(), "addr"), "addr can be a var");
}

// ---------------------------------------------------------------------------
// Errors
// ---------------------------------------------------------------------------

static void testMissingEndReportsOpeningLine(TestRunner &runner)
{
  runner.setTest("Parser: missing end");

  auto result = parse("turn x on\nrepeat 3 times\n  toggle x\n");

  check(runner, !result.ok(), "error reported");
  check(runner, result.errors.size(), static_cast<size_t>(1), "one error");
  check(runner, result.errors[0].line == 2, "points to the repeat line");
  check(runner,
        result.errors[0].message.find("repeat") != std::string::npos,
        "message mentions repeat");
}

static void testStrayEnd(TestRunner &runner)
{
  runner.setTest("Parser: stray end / else");

  auto result = parse("end\nelse\n");

  check(runner, !result.ok(), "errors reported");
  check(runner, result.errors.size(), static_cast<size_t>(2), "two errors");
  check(runner, result.errors[0].line == 1, "end on line 1");
  check(runner, result.errors[1].line == 2, "else on line 2");
}

static void testMultipleErrorsCollected(TestRunner &runner)
{
  runner.setTest("Parser: multiple errors collected");

  auto result = parse("blink x\n"
                      "turn light on\n"
                      "wait 5\n");

  check(runner, !result.ok(), "errors reported");
  check(runner, result.errors.size(), static_cast<size_t>(2), "two errors");
  check(runner, result.errors[0].line == 1, "first on line 1");
  check(runner, result.errors[1].line == 3, "second on line 3");
}

static void testReservedNameRejected(TestRunner &runner)
{
  runner.setTest("Parser: reserved word as name");

  auto result = parse("led if on pin 3\n");

  check(runner, !result.ok(), "error reported");
  check(runner,
        result.errors[0].message.find("reservada") != std::string::npos,
        "message mentions reserved word");
}

static void testArrowHint(TestRunner &runner)
{
  runner.setTest("Parser: old arrow gets a hint");

  auto result = parse("0 -> counter\n");

  check(runner, !result.ok(), "error reported");
  check(runner, result.errors[0].message.find("set") != std::string::npos,
        "hint mentions set");
}

static void testTooDeepNesting(TestRunner &runner)
{
  runner.setTest("Parser: nesting limit");

  std::string src;
  for (int i = 0; i < 18; i++)
    src += "if a == 1\n";
  src += "turn x on\n";
  for (int i = 0; i < 18; i++)
    src += "end\n";

  auto result = parse(src);

  check(runner, !result.ok(), "error reported");
  bool found = false;
  for (const auto &err : result.errors)
    if (err.message.find("anidados") != std::string::npos)
      found = true;
  check(runner, found, "message mentions nesting");
}

static void testErrorsDoNotRunOver(TestRunner &runner)
{
  runner.setTest("Parser: garbage after statement");

  auto result = parse("turn light on off\n");

  check(runner, !result.ok(), "error reported");
  check(runner,
        result.errors[0].message.find("sobra") != std::string::npos,
        "message mentions extra text");
}

static void testFullProgram(TestRunner &runner)
{
  runner.setTest("Parser: full example program");

  auto result = parse("# programa de ejemplo\n"
                      "led light on pin 17\n"
                      "button btn on pin 32\n"
                      "\n"
                      "set count to 0\n"
                      "log to \"sesion.log\"\n"
                      "\n"
                      "when btn pressed\n"
                      "  set count to count + 1\n"
                      "  print \"pulsado\" count\n"
                      "  if count >= 10\n"
                      "    show \"limite!\"\n"
                      "    stop\n"
                      "  end\n"
                      "end\n"
                      "\n"
                      "every 1 s\n"
                      "  toggle light\n"
                      "end\n");

  check(runner, result.ok(), "no errors");
  check(runner, result.program.top.size(), static_cast<size_t>(6),
        "six top-level statements");
  check(runner, result.program.top[4].kind == Stmt::Kind::When, "when");
  check(runner, result.program.top[5].kind == Stmt::Kind::Every, "every");

  const Stmt &when = result.program.top[4];
  check(runner, when.body.size(), static_cast<size_t>(3), "when body");
  check(runner, when.body[2].kind == Stmt::Kind::If, "if inside when");
  check(runner, when.body[2].branches[0].second[1].kind == Stmt::Kind::Stop,
        "stop inside nested if");
}

void run_parser_tests(TestRunner &runner)
{
  testDeviceDeclarations(runner);
  testTurnToggle(runner);
  testAssignmentForms(runner);
  testWaitDurations(runner);
  testWaitWithoutUnitFails(runner);
  testPrintShowLog(runner);

  testArithmeticPrecedence(runner);
  testLogicalPrecedence(runner);
  testNotBindsLooserThanComparison(runner);
  testComparisonOverArithmetic(runner);
  testIsForms(runner);
  testSigned16AndUnaryMinus(runner);
  testParenthesesOverridePrecedence(runner);

  testIfElseChain(runner);
  testNestedBlocks(runner);
  testRepeatForms(runner);
  testWhenEvery(runner);

  testI2cWrite(runner);
  testI2cRead(runner);

  testMissingEndReportsOpeningLine(runner);
  testStrayEnd(runner);
  testMultipleErrorsCollected(runner);
  testReservedNameRejected(runner);
  testArrowHint(runner);
  testTooDeepNesting(runner);
  testErrorsDoNotRunOver(runner);
  testFullProgram(runner);
}
