#pragma once

#include <vector>
#include <dirent.h>

#include "app.hpp"
#include "blackboard.hpp"
#include "stride_locator.hpp"
#include "stride_subscription.hpp"

class AppManager
{
public:
  static AppManager &Instance();

  void scan();
  void register_app(const AppDescriptor &app);

  const std::vector<AppDescriptor> &apps() const { return _apps; }
  int version() const { return _version; }

private:
  AppManager();

  void rebuild_apps();

  std::vector<AppDescriptor> _apps;
  std::vector<AppDescriptor> _builtins;
  std::vector<AppDescriptor> _scripts;
  int _version = 0;
  StrideSubscription _file_subscription;
};
