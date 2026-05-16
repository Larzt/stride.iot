#include "main_state.hpp"
#include "running_state.hpp"
#include "app_manager.hpp"
#include "stride_locator.hpp"

#include <cctype>
#include <cstdio>
#include <cstdlib>

void MainState::on_enter(Display &ctx)
{
  auto *am = StrideLocator::Get<AppManager>();
  if (am)
  {
    _apps = am->apps();
    _app_version = am->version();
  }

  auto &tft = ctx.getTFT();
  tft.fillScreen(TFT_BLACK);

  draw_header(ctx);
  draw_files(ctx);

  _ip_subscription = Blackboard::LocalIpAddress.subscribe([this](const std::string &)
                                                          { _ip_dirty = true; });
  _wifi_subscription = Blackboard::WifiIpAddress.subscribe([this](const std::string &)
                                                           { _ip_dirty = true; });
  _mode_subscription = Blackboard::CurrentNetworkMode.subscribe([this](NetworkMode)
                                                                { _ip_dirty = true; });
  _file_subscription = Blackboard::FileListVersion.subscribe([this](int)
                                                             { _files_dirty = true; });
  _running_subscription = Blackboard::RunningProgramName.subscribe([this](const std::string &name)
                                                                   { if (!name.empty()) _running_dirty = true; });

  if (!Blackboard::RunningProgramName.get().empty())
    _running_dirty = true;

  _cursor_subscription = _cursor_position.subscribe([this, &ctx](int)
                                                    {
    _cursor_anim_x   = 0;
    _cursor_anim_fwd = true;
    draw_files(ctx);
    if (_apps.empty()) return;
    int idx = _cursor_position.get();
    if (idx >= 0 && idx < (int)_apps.size())
      Blackboard::CurrentProgram = _apps[idx]; });

  std::string current_ip = get_active_ip();
  if (!current_ip.empty())
  {
    draw_ip_footer(ctx, current_ip);
  }
}

void MainState::on_exit(Display &ctx)
{
  _cursor_subscription.unsubscribe();
  _ip_subscription.unsubscribe();
  _wifi_subscription.unsubscribe();
  _mode_subscription.unsubscribe();
  _file_subscription.unsubscribe();
  _running_subscription.unsubscribe();
}

void MainState::on_update(Display &ctx)
{
  if (_running_dirty)
  {
    _running_dirty = false;
    Display::Instance().transition_to(std::make_unique<RunningState>());
    return;
  }

  if (_files_dirty || lgfx::millis() - _last_scan_ms > 2000)
  {
    _files_dirty = false;
    _last_scan_ms = lgfx::millis();
    reload_files(ctx);
  }

  if (_ip_dirty)
  {
    _ip_dirty = false;
    std::string ip = get_active_ip();
    if (!ip.empty())
    {
      draw_ip_footer(ctx, ip);
    }
    else
    {
      auto &tft = ctx.getTFT();
      tft.fillRect(0, tft.height() - FOOTER_H, tft.width(), FOOTER_H, TFT_BLACK);
    }
  }

  if (!_apps.empty() && lgfx::millis() - _last_cursor_anim_ms > 100)
  {
    _last_cursor_anim_ms = lgfx::millis();
    if (_cursor_anim_fwd)
    {
      if (++_cursor_anim_x >= 4) _cursor_anim_fwd = false;
    }
    else
    {
      if (--_cursor_anim_x <= 0) _cursor_anim_fwd = true;
    }
    draw_cursor_anim(ctx);
  }

  if (get_active_ip().empty() && lgfx::millis() - _last_anim_ms > 100)
  {
    _last_anim_ms = lgfx::millis();

    char fake_ip[16];
    std::snprintf(fake_ip, sizeof(fake_ip), "%d.%d.%d.%d",
                  std::abs(rand() % 256),
                  std::abs(rand() % 256),
                  std::abs(rand() % 256),
                  std::abs(rand() % 256));

    draw_ip_footer(ctx, fake_ip);
  }
}

void MainState::on_input(const InputEvent &event)
{
  if (event.type == InputType::Button)
  {
    move_cursor_position(event.delta);
  }
}

void MainState::draw_header(Display &ctx)
{
  auto &tft = ctx.getTFT();
  int32_t w = tft.width();

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawCenterString("STRIDE", w / 2, 7);

  tft.drawFastHLine(4, 27, w - 8, TFT_DARKGREY);
}

void MainState::draw_files(Display &ctx)
{
  auto &tft = ctx.getTFT();
  int32_t w = tft.width();
  int32_t h = tft.height();
  int list_bottom = h - FOOTER_H;
  int sel = _cursor_position.get();
  int y = LIST_Y;

  if (_apps.empty())
  {
    tft.fillRect(0, LIST_Y, w, list_bottom - LIST_Y, TFT_BLACK);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.setTextSize(1);
    tft.drawCenterString("Sin archivos", w / 2, LIST_Y + (list_bottom - LIST_Y) / 2 - 4);
    return;
  }

  for (size_t i = 0; i < _apps.size() && y < list_bottom; i++, y += LINE_H)
  {
    bool selected = ((int)i == sel);

    uint32_t bg = selected ? TFT_NAVY : TFT_BLACK;
    uint32_t fg = selected ? TFT_WHITE : TFT_DARKGREY;

    tft.fillRect(0, y, w, LINE_H - 1, bg);
    tft.setTextColor(fg, bg);
    tft.setTextSize(1);

    std::string name = _apps[i].name;
    std::string ext;
    size_t dot = name.find_last_of('.');
    if (dot != std::string::npos)
    {
      ext  = name.substr(dot + 1);
      name = name.substr(0, dot);
      for (auto &c : ext) c = (char)tolower((unsigned char)c);
    }

    tft.drawString(name.c_str(), 16, y + 4);
    if (selected)
      tft.drawString(">", 4 + _cursor_anim_x, y + 4);

    if (!ext.empty())
    {
      uint32_t badge_color = (ext == "str") ? 0x07E0u
                           : (ext == "log") ? 0xFD20u
                                            : (uint32_t)TFT_DARKGREY;
      tft.setTextColor(badge_color, bg);
      std::string badge = ext;
      for (auto &c : badge) c = (char)toupper((unsigned char)c);
      tft.drawRightString(badge.c_str(), w - 4, y + 4);
    }
  }

  if (y < list_bottom)
    tft.fillRect(0, y, w, list_bottom - y, TFT_BLACK);
}

void MainState::draw_cursor_anim(Display &ctx)
{
  int sel = _cursor_position.get();
  if (sel < 0 || sel >= (int)_apps.size()) return;

  auto &tft = ctx.getTFT();
  int y = LIST_Y + sel * LINE_H;
  if (y >= tft.height() - FOOTER_H) return;

  tft.fillRect(4, y + 1, 11, LINE_H - 3, TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString(">", 4 + _cursor_anim_x, y + 4);
}

void MainState::draw_ip_footer(Display &ctx, const std::string &text)
{
  auto &tft = ctx.getTFT();
  int32_t w = tft.width();
  int32_t h = tft.height();
  int32_t footer_y = h - FOOTER_H;

  tft.fillRect(0, footer_y, w, FOOTER_H, TFT_BLACK);
  tft.drawFastHLine(4, footer_y + 1, w - 8, TFT_DARKGREY);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(1);
  tft.drawCenterString(text.c_str(), w / 2, footer_y + 9);
}

void MainState::move_cursor_position(int delta)
{
  if (_apps.empty())
  {
    return;
  }

  int n = (int)_apps.size();
  int new_pos = (_cursor_position.get() + delta % n + n) % n;
  _cursor_position.set(new_pos);
}

void MainState::reload_files(Display &ctx)
{
  auto *am = StrideLocator::Get<AppManager>();
  if (!am) return;

  int current_version = am->version();
  if (current_version == _app_version) return;
  _app_version = current_version;

  _apps = am->apps();

  auto &tft = ctx.getTFT();
  tft.fillRect(0, LIST_Y, tft.width(), tft.height() - FOOTER_H - LIST_Y, TFT_BLACK);

  int idx = _cursor_position.get();
  if (!_apps.empty() && idx >= (int)_apps.size())
  {
    _cursor_position.set((int)_apps.size() - 1);
    return;
  }

  draw_files(ctx);

  if (!_apps.empty() && idx >= 0 && idx < (int)_apps.size())
    Blackboard::CurrentProgram = _apps[idx];
}

std::string MainState::get_active_ip() const
{
  NetworkMode mode = Blackboard::CurrentNetworkMode.get();

  auto valid = [](const std::string &ip)
  {
    return !ip.empty() && ip != "0.0.0.0";
  };

  if (mode == NetworkMode::Station)
  {
    std::string ip = Blackboard::WifiIpAddress.get();
    if (valid(ip))
    {
      return ip;
    }
  }
  else
  {
    std::string ip = Blackboard::LocalIpAddress.get();
    if (valid(ip))
    {
      return ip;
    }
  }

  return "";
}
