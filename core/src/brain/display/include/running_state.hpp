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
  }

  void on_exit(Display &ctx) override
  {
    _running_subscription.unsubscribe();
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

  std::string _program_name;
  uint32_t _start_ms = 0;
  uint32_t _last_anim_ms = 0;
  int _spinner_step = 0;
  volatile bool _finished = false;
  StrideSubscription _running_subscription;
};
