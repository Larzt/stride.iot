#pragma once
#include "display.hpp"

#include <dirent.h>
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
  void on_enter(Display &ctx) override
  {
    AppDescriptor app;

    app.name = "IMU Monitor";
    app.path = "";
    app.type = AppType::Builtin;
    _apps.push_back(app);

    // Read card files
    DIR *dir = opendir(Blackboard::MountPoint.c_str());
    if (dir)
    {
      struct dirent *entry;
      while ((entry = readdir(dir)) != NULL)
      {
        std::string name = "/" + std::string(entry->d_name);
        std::string extension = name.substr(name.length() - 4);
        if (extension != ".log" && extension != ".LOG" && extension != ".str" && extension != ".STR")
        {
          continue;
        }
        app.name = name;
        app.path = name;
        app.type = AppType::Script;
        _apps.push_back(app);
      }
      closedir(dir);
    }

    auto &tft = ctx.getTFT();
    int32_t width = tft.width();
    int32_t height = tft.height();

    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_YELLOW);
    tft.setTextSize(2);
    tft.drawCenterString("Programas", width / 2, 10);
    tft.drawCenterString("cargados", width / 2, 30);

    draw_files(ctx);

    tft.setTextColor(TFT_YELLOW);
    tft.setTextSize(2);
    // tft.drawWedgeLine(0, height-30, width, height-30, 1.5, 1.5, TFT_WHITE);
    tft.drawCenterString(Blackboard::LocalIpAddress.get().c_str(), width / 2, height - 20);

    _cursor_subscription = _cursor_position.subscribe([this, &ctx](int)
                                                      {
        draw_files(ctx);

        if (_apps.empty()) return;
        int idx = _cursor_position.get();
        if (idx < 0 || idx >= (int)_apps.size()) return;
        Blackboard::CurrentProgram = _apps[idx]; });

    _ip_subscription = Blackboard::LocalIpAddress.subscribe([this, &ctx](const std::string &ip)
                                                            {
        auto &tft = ctx.getTFT();
        int32_t width  = tft.width();
        int32_t height = tft.height();

        tft.fillRect(0, height - 30, width, 30, TFT_BLACK);
        tft.setTextColor(TFT_YELLOW);
        tft.setTextSize(2);
        tft.drawCenterString(ip.c_str(), width / 2, height - 20); });
  }

  void on_exit(Display &ctx) override
  {
    _cursor_subscription.unsubscribe();
    _ip_subscription.unsubscribe();
  }

  void on_update(Display &ctx) override {}

  void on_input(const InputEvent &event) override
  {
    if (event.type == InputType::Button)
    {
      int value = event.delta;
      move_cursor_position(value);
    }
  }

  void draw_files(Display &ctx)
  {
    auto &tft = ctx.getTFT();

    tft.setTextSize(1);

    int y = 51;
    int lineHeight = 10;

    for (size_t i = 0; i < _apps.size(); i++)
    {
      if (i == (size_t)_cursor_position.get())
      {
        tft.setTextColor(TFT_ALICEBLUE, TFT_BLACK);
      }
      else
      {
        tft.setTextColor(TFT_BLACK, TFT_YELLOW);
      }

      tft.drawString(_apps[i].name.c_str(), 10, y);
      y += lineHeight;

      if (y > tft.height() - 20)
        break;
    }
  }

  void move_cursor_position(int delta)
  {
    if (_apps.empty())
    {
      return;
    }

    _cursor_position.set(_cursor_position.get() + delta);

    if (_cursor_position.get() < 0)
    {
      _cursor_position.set(_apps.size() - 1);
    }

    if (_cursor_position.get() >= (int)_apps.size())
    {
      _cursor_position.set(0);
    }
  }

  StateType get_type() const override { return StateType::Main; }

private:
  std::vector<AppDescriptor> _apps;
  StrideObservable<int> _cursor_position{0};
  StrideSubscription _cursor_subscription;
  StrideSubscription _ip_subscription;
};
