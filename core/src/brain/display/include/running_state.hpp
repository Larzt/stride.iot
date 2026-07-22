#pragma once

#include "display.hpp"
#include "main_state.hpp"
#include "blackboard.hpp"
#include "enums.hpp"
#include "stride_subscription.hpp"

#include <string>

class RunningState : public DisplayBaseState
{
public:
  void on_enter(Display &ctx) override
  {
    _program_name = Blackboard::RunningProgramName.get();
    _start_ms = lgfx::millis();

    draw_static(ctx);

    _running_subscription = Blackboard::RunningProgramName.subscribe(
        [this](const std::string &name)
        {
          if (name.empty())
            _finished = true;
        });

    _show_subscription = Blackboard::DslShowText.subscribe(
        [this](const std::string &message)
        {
          if (message.empty())
            return;
          for (int i = 0; i < kShowLines - 1; i++)
            _show_lines[i] = _show_lines[i + 1];
          _show_lines[kShowLines - 1] = message;
          _show_dirty = true;
        });
  }

  void on_update(Display &ctx) override
  {
    if (_finished)
    {
      Display::Instance().transition_to(std::make_unique<MainState>());
      return;
    }

    if (lgfx::millis() - _last_anim_ms < 100)
      return;
    _last_anim_ms = lgfx::millis();

    _spinner_step = (_spinner_step + 1) % 8;
    draw_spinner(ctx);
    draw_elapsed(ctx);

    if (_show_dirty)
    {
      _show_dirty = false;
      draw_show_lines(ctx);
    }
  }

  void on_exit(Display &ctx) override
  {
    _running_subscription.unsubscribe();
    _show_subscription.unsubscribe();
  }

  StateType get_type() const override { return StateType::Running; }

private:
  void draw_static(Display &ctx)
  {
    auto &tft = ctx.getTFT();
    int32_t w = tft.width();

    tft.fillScreen(TFT_BLACK);

    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextSize(2);
    tft.drawCenterString("Ejecutando", w / 2, 20);

    tft.drawFastHLine(8, 44, w - 16, TFT_DARKGREY);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);

    std::string name = _program_name;
    if (name.size() > 26)
      name = name.substr(0, 23) + "...";
    tft.drawCenterString(name.c_str(), w / 2, 56);
  }

  void draw_spinner(Display &ctx)
  {
    auto &tft = ctx.getTFT();
    int cx = tft.width() / 2;
    int cy = 120;
    int r_out = 16;
    int r_in = 10;

    tft.fillCircle(cx, cy, r_out + 1, TFT_BLACK);

    int head = _spinner_step * 45;
    int tail = (head - 90 + 360) % 360;
    tft.fillArc(cx, cy, r_in, r_out, tail, head, TFT_GREEN);
  }

  void draw_elapsed(Display &ctx)
  {
    auto &tft = ctx.getTFT();
    int32_t w = tft.width();
    int32_t h = tft.height();
    uint32_t secs = (lgfx::millis() - _start_ms) / 1000;

    char buf[24];
    std::snprintf(buf, sizeof(buf), "%lus", (unsigned long)secs);

    tft.fillRect(0, h - 24, w, 16, TFT_BLACK);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextSize(1);
    tft.drawCenterString(buf, w / 2, h - 20);
  }

  // Last lines emitted by the DSL `show` command, below the spinner.
  void draw_show_lines(Display &ctx)
  {
    auto &tft = ctx.getTFT();
    int32_t w = tft.width();

    tft.fillRect(0, 146, w, kShowLines * 12 + 4, TFT_BLACK);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setTextSize(1);

    for (int i = 0; i < kShowLines; i++)
    {
      std::string line = _show_lines[i];
      if (line.size() > 28)
        line = line.substr(0, 25) + "...";
      tft.drawCenterString(line.c_str(), w / 2, 148 + i * 12);
    }
  }

  static constexpr int kShowLines = 4;

  std::string _program_name;
  uint32_t _start_ms = 0;
  uint32_t _last_anim_ms = 0;
  int _spinner_step = 0;
  volatile bool _finished = false;
  volatile bool _show_dirty = false;
  std::string _show_lines[kShowLines];
  StrideSubscription _running_subscription;
  StrideSubscription _show_subscription;
};
