#pragma once
#include "display.hpp"
#include "main_state.hpp"

class StartupState : public DisplayBaseState
{
public:
  void on_enter(Display &ctx) override
  {
    auto &tft = ctx.getTFT();
    _startTime = lgfx::millis();

    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_YELLOW);
    tft.setTextSize(2);

    tft.drawCenterString("Bienvenido a", tft.width() / 2, 60);
    tft.setTextSize(3);
    tft.drawCenterString("Stride", tft.width() / 2, 90);

    tft.fillCircle(tft.width() / 2, 160, 22, TFT_BLACK);
  }

  void on_update(Display &ctx) override
  {
    auto &tft = ctx.getTFT();
    uint32_t elapsed = lgfx::millis() - _startTime;

    if (elapsed >= Blackboard::LoadingTimeMs)
    {
      Display::Instance().transition_to(std::make_unique<MainState>());
      return;
    }

    int centerX = tft.width() / 2;
    int centerY = 160;
    int rIn = 14;
    int rOut = 20;
    int segmentLen = 60;

    int headAngle = (elapsed / 2) % 360;
    int tailAngle = (headAngle - segmentLen + 360) % 360;

    tft.fillArc(centerX, centerY, rIn - 1, rOut + 1, headAngle, tailAngle, TFT_BLACK);

    tft.fillArc(centerX, centerY, rIn, rOut, tailAngle, headAngle, TFT_YELLOW);
  }

  void on_exit(Display &ctx) override {}
  StateType get_type() const override { return StateType::Startup; }

private:
  uint32_t _startTime = 0;
};
