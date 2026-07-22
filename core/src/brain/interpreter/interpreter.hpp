#pragma once

#include <atomic>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "driver/i2c_master.h"
#include "expander_task.hpp"
#include "i2c_bus.hpp"

#include "lang_ast.hpp"

#include "blackboard.hpp"
#include "types.hpp"

#include "stride_button.hpp"
#include "stride_buzzer.hpp"
#include "stride_led.hpp"
#include "stride_logger.hpp"

class Interpreter
{
public:
  static Interpreter &Instance()
  {
    static Interpreter instance;
    return instance;
  }

  // Runs a parsed program: top-level statements first, then (if any when/
  // every handlers were registered) the event dispatch loop. The registered
  // handlers keep pointers into `program`, which the caller owns; execute()
  // is synchronous, so a Program living in the caller's frame is safe.
  void execute(const StrideProgram &program);

  inline void start_endless_loop() { endless_loop.store(true); }
  inline void stop_endless_loop() { endless_loop.store(false); }

private:
  Interpreter() = default;
  ~Interpreter() = default;

  Interpreter(const Interpreter &) = delete;
  Interpreter &operator=(const Interpreter &) = delete;

  // Every script device (GPIO led/buzzer/button or expander pin) behind one
  // interface, so `turn X on` and reads work the same for all of them.
  struct DeviceEntry
  {
    lang::DeviceType type = lang::DeviceType::Led;
    StrideLed *led = nullptr;
    StrideBuzzer *buzzer = nullptr;
    StrideButton *button = nullptr;
    uint8_t expander_pin = 0;
  };

  struct Handler
  {
    const lang::Stmt *stmt = nullptr;
    StrideButton *button = nullptr; // when
    uint32_t period_ms = 0;         // every
    uint32_t next_due = 0;
  };

  // interpreter.cc
  void reset_run_state();
  void run_handlers();
  void register_when(const lang::Stmt &stmt);
  void register_every(const lang::Stmt &stmt);
  bool should_stop() const;
  void runtime_error(uint16_t line, const std::string &message);

  // interpreter_exec.cc
  void exec_block(const lang::Block &block);
  void exec_stmt(const lang::Stmt &stmt);
  void exec_if(const lang::Stmt &stmt);
  void exec_repeat(const lang::Stmt &stmt);
  void exec_wait(const lang::Stmt &stmt);
  void exec_print(const lang::Stmt &stmt);
  void exec_show(const lang::Stmt &stmt);
  void exec_log_to(const lang::Stmt &stmt);
  void interruptible_delay(uint32_t ms);
  std::string build_output_message(const lang::Stmt &stmt);
  uint32_t resolve_duration_ms(const lang::Stmt &stmt);

  // interpreter_expr.cc
  int32_t eval(const lang::Expr *expr);

  // interpreter_devices.cc
  void declare_device(const lang::Stmt &stmt);
  void exec_turn(const lang::Stmt &stmt);
  void exec_toggle(const lang::Stmt &stmt);
  DeviceEntry *find_device(const std::string &name);
  void device_set(DeviceEntry &device, bool value, uint16_t line);
  int32_t device_get(DeviceEntry &device, uint16_t line);
  void clear_devices();

  // interpreter_i2c.cc
  void exec_i2c_write(const lang::Stmt &stmt);
  void exec_i2c_read(const lang::Stmt &stmt);
  bool i2c_ensure_initialized(uint16_t line);
  i2c_master_dev_handle_t i2c_get_or_create_device(uint8_t addr,
                                                   uint32_t speed_hz = 100000);

  std::map<std::string, DeviceEntry> _devices;
  std::map<std::string, int32_t> _variables;

  std::vector<Handler> _when_handlers;
  std::vector<Handler> _every_handlers;

  std::map<uint8_t, i2c_master_dev_handle_t> _i2c_devices;
  bool _i2c_initialized = false;

  bool _stop_requested = false;
  std::atomic<bool> endless_loop{true};
};
