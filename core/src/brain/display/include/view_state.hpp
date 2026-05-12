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
  explicit ViewState(const std::string &file)
      : _file(file)
  {
  }

  void on_enter(Display &ctx) override
  {
    load_file();

    auto &tft = ctx.getTFT();

    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_YELLOW);
    tft.setTextSize(1);

    draw(ctx);
  }

  void on_update(Display &ctx) override
  {
  }

  void on_input(const InputEvent &event) override
  {
    if (event.type == InputType::Button && event.delta == 0)
    {
      Display::Instance().transition_to(std::make_unique<MainState>());
    }

    _scroll += event.delta;

    if (_scroll < 0)
      _scroll = 0;

    if (_scroll >= (int)_lines.size())
      _scroll = _lines.size() - 1;

    draw(Display::Instance());
  }

  void on_exit(Display &ctx) override
  {
  }

  StateType get_type() const override
  {
    return StateType::View;
  }

private:
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

    char buffer[128];

    while (fgets(buffer, sizeof(buffer), file))
    {
      std::string line(buffer);

      // remove \n
      if (!line.empty() && line.back() == '\n')
      {
        line.pop_back();
      }

      _lines.push_back(line);
    }

    fclose(file);

    if (_lines.empty())
    {
      _lines.push_back("(empty file)");
    }
  }

  void draw(Display &ctx)
  {
    auto &tft = ctx.getTFT();

    tft.fillScreen(TFT_BLACK);

    tft.setTextColor(TFT_YELLOW);
    tft.setTextSize(1);

    int y = 0;
    int lineHeight = 10;

    for (size_t i = _scroll; i < _lines.size(); i++)
    {
      tft.drawString(_lines[i].c_str(), 0, y);

      y += lineHeight;

      if (y > tft.height() - lineHeight)
      {
        break;
      }
    }
  }

private:
  std::string _file;
  std::vector<std::string> _lines;
  int _scroll = 0;
};
