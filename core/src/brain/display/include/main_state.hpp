#pragma once
#include "display.hpp"

#include <string>
#include <vector>

#include "enums.hpp"
#include "app.hpp"
#include "stride_logger.hpp"
#include "stride_observer.hpp"
#include "stride_subscription.hpp"

class MainState : public DisplayBaseState
{
public:
  void on_enter(Display &ctx) override;
  void on_exit(Display &ctx) override;
  void on_update(Display &ctx) override;
  void on_input(const InputEvent &event) override;

  StateType get_type() const override { return StateType::Main; }

private:
  void draw_header(Display &ctx);
  void draw_files(Display &ctx);
  void draw_cursor_anim(Display &ctx);
  void draw_ip_footer(Display &ctx, const std::string &text);
  void move_cursor_position(int delta);
  void reload_files(Display &ctx);
  std::string get_active_ip() const;

  static constexpr int LIST_Y   = 31;
  static constexpr int LINE_H   = 16;
  static constexpr int FOOTER_H = 24;

  volatile bool _ip_dirty = false;
  volatile bool _files_dirty = false;
  int _app_version = -1;
  uint32_t _last_anim_ms = 0;
  uint32_t _last_scan_ms = 0;
  uint32_t _last_cursor_anim_ms = 0;
  int  _cursor_anim_x   = 0;
  bool _cursor_anim_fwd = true;
  std::vector<AppDescriptor> _apps;
  StrideObservable<int> _cursor_position{0};
  StrideSubscription _cursor_subscription;
  StrideSubscription _ip_subscription;
  StrideSubscription _wifi_subscription;
  StrideSubscription _mode_subscription;
  StrideSubscription _file_subscription;
};
