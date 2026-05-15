#pragma once

#include <string>
#include <vector>
#include <map>
#include <atomic>
#include <sstream>

#include "driver/i2c_master.h"
#include "i2c_bus.hpp"

#include "lexer.hpp"
#include "sink.hpp"
#include "hexer.hpp"

#include "blackboard.hpp"
#include "types.hpp"

#include "stride_led.hpp"
#include "stride_button.hpp"
#include "stride_buzzer.hpp"
#include "stride_logger.hpp"
#include "timer.hpp"

class Interpreter
{
public:
  static Interpreter &Instance()
  {
    static Interpreter instance;
    return instance;
  }

  void execute(const StrideProgram &program);

  inline void start_endless_loop() { endless_loop.store(true); }
  inline void stop_endless_loop() { endless_loop.store(false); }

private:
  Interpreter() = default;
  ~Interpreter() = default;

  Interpreter(const Interpreter &) = delete;
  Interpreter &operator=(const Interpreter &) = delete;

  void executeLogfile(const std::vector<Token> &tokens);
  void execute_simple_block_command(const std::vector<Token> &tokens);
  void execute_range(const StrideProgram &program, size_t start, size_t end);

  // Simple commands
  void load_device_command(const std::vector<Token> &tokens);
  int resolve_value(const Token &token);
  void execute_write_command(const std::vector<Token> &tokens);
  void execute_wait_command(const std::vector<Token> &tokens);
  void execute_print_command(const std::vector<Token> &tokens);

  // Assignation
  void execute_arrow_allocation(const std::vector<Token> &tokens);
  void execute_simple_allocation(const std::vector<Token> &tokens);
  void execute_expr_allocation(const std::vector<Token> &tokens);
  int resolve_expr_token(const Token &token);
  int eval_expr(const std::vector<Token> &tokens, size_t &pos);
  int eval_primary(const std::vector<Token> &tokens, size_t &pos);

  // Control
  bool evaluate_condition(const std::vector<Token> &tokens);
  size_t execute_control_loop(const StrideProgram &program, size_t index);
  size_t execute_control_if(const StrideProgram &program, size_t index);

  // I2C
  void executeI2C(const std::vector<Token> &tokens);
  void executeI2CInit(const std::vector<Token> &tokens);
  void executeI2CWrite(const std::vector<Token> &tokens);
  void executeI2CRead(const std::vector<Token> &tokens);
  void executeI2CReadLE(const std::vector<Token> &tokens);
  i2c_master_dev_handle_t i2c_get_or_create_device(uint8_t addr, uint32_t speed_hz = 100000);

  // Unary commands
  void execute_sign16_command(const std::vector<Token> &tokens);

  std::map<uint8_t, i2c_master_dev_handle_t> _i2c_devices;
  bool _i2c_initialized = false;

  // Variables
  StrideVariable<int> _variables;
  StrideVariable<StrideLed *> _leds;
  StrideVariable<StrideBuzzer *> _buzzers;
  StrideVariable<StrideButton *> _buttons;
  std::atomic<bool> endless_loop{true};
};
