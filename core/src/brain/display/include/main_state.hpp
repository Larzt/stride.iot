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

    auto ip_callback = [this, &ctx](const std::string &ip)
    {
      if (!ip.empty() && ip != "0.0.0.0")
      {
        draw_ip_footer(ctx, ip);
      }
    };

    _ip_subscription = Blackboard::LocalIpAddress.subscribe(ip_callback);
    _wifi_subscription = Blackboard::WifiIpAddress.subscribe(ip_callback);

    _mode_subscription = Blackboard::CurrentNetworkMode.subscribe([this, &ctx](NetworkMode mode)
                                                                  {
        std::string active_ip = get_active_ip();
        if (!active_ip.empty()) {
            draw_ip_footer(ctx, active_ip);
        } else {
            auto &tft = ctx.getTFT();
            tft.fillRect(0, tft.height() - 25, tft.width(), 25, TFT_BLACK);
        } });

    _cursor_subscription = _cursor_position.subscribe([this, &ctx](int)
                                                      {
        draw_files(ctx);
        if (_apps.empty()) return;
        int idx = _cursor_position.get();
        if (idx < 0 || idx >= (int)_apps.size()) return;
        Blackboard::CurrentProgram = _apps[idx]; });

    std::string current_ip = get_active_ip();
    if (!current_ip.empty())
    {
      draw_ip_footer(ctx, current_ip);
    }
  }

  void on_exit(Display &ctx) override
  {
    _cursor_subscription.unsubscribe();
    _ip_subscription.unsubscribe();
    _wifi_subscription.unsubscribe();
    _mode_subscription.unsubscribe();
  }

  void on_update(Display &ctx) override
  {
    std::string active_ip = get_active_ip();

    if (active_ip.empty())
    {
      if (lgfx::millis() - _last_anim_ms > 100)
      {
        _last_anim_ms = lgfx::millis();

        char fake_ip[16];
        sprintf(fake_ip, "%d.%d.%d.%d",
                abs(rand() % 256),
                abs(rand() % 256),
                abs(rand() % 256),
                abs(rand() % 256));

        draw_ip_footer(ctx, fake_ip);
      }
    }
  }

  void on_input(const InputEvent &event) override
  {
    if (event.type == InputType::Button)
    {
      int value = event.delta;
      move_cursor_position(value);
    }
  }

  StateType get_type() const override { return StateType::Main; }

private:
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

      if (y > tft.height() - 30)
        break;
    }
  }

  void move_cursor_position(int delta)
  {
    if (_apps.empty())
    {
      return;
    }

    int new_pos = _cursor_position.get() + delta;
    if (new_pos < 0)
    {
      new_pos = _apps.size() - 1;
    }
    if (new_pos >= (int)_apps.size())
    {
      new_pos = 0;
    }

    _cursor_position.set(new_pos);
  }

  void draw_ip_footer(Display &ctx, std::string text)
  {
    auto &tft = ctx.getTFT();
    int32_t width = tft.width();
    int32_t height = tft.height();

    tft.fillRect(0, height - 25, width, 25, TFT_BLACK);
    tft.setTextColor(TFT_YELLOW);
    tft.setTextSize(1.75);
    tft.drawCenterString(text.c_str(), width / 2, height - 20);
  }

  std::string get_active_ip()
  {
    NetworkMode mode = Blackboard::CurrentNetworkMode.get();

    if (mode == NetworkMode::Station)
    {
      std::string wifi = Blackboard::WifiIpAddress.get();
      if (!wifi.empty() && wifi != "0.0.0.0")
      {
        return wifi;
      }
    }
    else if (mode == NetworkMode::Access)
    {
      std::string local = Blackboard::LocalIpAddress.get();
      if (!local.empty() && local != "0.0.0.0")
      {
        return local;
      }
    }

    return "";
  }

  uint32_t _last_anim_ms = 0;
  std::vector<AppDescriptor> _apps;
  StrideObservable<int> _cursor_position{0};
  StrideSubscription _cursor_subscription;
  StrideSubscription _ip_subscription;
  StrideSubscription _wifi_subscription;
  StrideSubscription _mode_subscription;
};
