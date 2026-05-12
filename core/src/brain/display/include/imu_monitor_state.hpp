#pragma once
#include "display.hpp"
#include "main_state.hpp"
#include "imu_task.hpp"

class ImuMonitorState : public DisplayBaseState
{
public:
  void on_enter(Display &ctx) override
  {
    auto &tft = ctx.getTFT();
    tft.fillScreen(TFT_BLACK);

    imu_enable();
  }

  void on_update(Display &ctx) override
  {
    auto &tft = ctx.getTFT();

    tft.fillScreen(TFT_BLACK);

    tft.setCursor(0, 0);

    tft.printf("ROLL : %.2f\n", Blackboard::ImuRoll);
    tft.printf("PITCH: %.2f\n", Blackboard::ImuPitch);
    tft.printf("YAW  : %.2f\n", Blackboard::ImuYaw);
  }

  void on_input(const InputEvent &event) override
  {
    if (event.type == InputType::Button)
    {
      Display::Instance().transition_to(std::make_unique<MainState>());
    }
  }

  void on_exit(Display &ctx) override
  {
    imu_disable();
  }

  StateType get_type() const override
  {
    return StateType::ImuMonitor;
  }
};
