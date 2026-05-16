#pragma once

#include "display.hpp"
#include "main_state.hpp"
#include "blackboard.hpp"
#include "enums.hpp"

#include <string>
#include <vector>
#include <cstdio>

class ViewState : public DisplayBaseState
{
public:
  explicit ViewState(const std::string &file) : _file(file) {}

  void on_enter(Display &ctx) override
  {
    load_file();

    auto &tft = ctx.getTFT();
    tft.fillScreen(TFT_BLACK);

    draw(ctx);
  }

  void on_update(Display &ctx) override {}

  void on_input(const InputEvent &event) override
  {
    if (event.type == InputType::Button && event.delta == 0)
    {
      Display::Instance().transition_to(std::make_unique<MainState>());
      return;
    }

    _scroll += event.delta;

    if (_scroll < 0)
      _scroll = 0;

    int max_scroll = (int)_lines.size() - 1;
    if (max_scroll < 0) max_scroll = 0;
    if (_scroll > max_scroll) _scroll = max_scroll;

    draw(Display::Instance());
  }

  void on_exit(Display &ctx) override {}

  StateType get_type() const override { return StateType::View; }

private:
  static constexpr int CHARS_PER_LINE = 28;

  void push_wrapped(const std::string &s)
  {
    if (s.empty())
    {
      _lines.push_back("");
      return;
    }
    for (size_t i = 0; i < s.size(); i += CHARS_PER_LINE)
      _lines.push_back(s.substr(i, CHARS_PER_LINE));
  }

  void load_file()
  {
    _lines.clear();

    std::string full_path = Blackboard::MountPoint + _file;
    FILE *file = fopen(full_path.c_str(), "r");
    if (!file)
    {
      _lines.push_back("Error opening file");
      return;
    }

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), file))
    {
      std::string line(buffer);
      if (!line.empty() && line.back() == '\n')
        line.pop_back();
      if (line.empty())
        continue;

      if (line[0] == '[')
      {
        size_t close = line.find(']');
        if (close != std::string::npos &&
            close + 2 < line.size() &&
            line[close + 1] == ':' &&
            line[close + 2] == ' ')
        {
          push_wrapped(line.substr(0, close + 1));
          push_wrapped(line.substr(close + 3));
          _lines.push_back("");
          continue;
        }
      }

      push_wrapped(line);
    }

    fclose(file);

    if (_lines.empty())
      _lines.push_back("(empty file)");
  }

  void draw(Display &ctx)
  {
    auto &tft = ctx.getTFT();
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextSize(1);

    int y = 0;
    const int lineHeight = 10;

    for (size_t i = (size_t)_scroll; i < _lines.size(); i++)
    {
      if (!_lines[i].empty())
        tft.drawString(_lines[i].c_str(), 0, y);
      y += lineHeight;
      if (y > tft.height() - lineHeight)
        break;
    }
  }

  std::string _file;
  std::vector<std::string> _lines;
  int _scroll = 0;
};
