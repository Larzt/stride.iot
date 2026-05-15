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

  const std::vector<AppDescriptor> &apps() const { return _apps; }
  int version() const { return _version; }

private:
  AppManager();

  std::vector<AppDescriptor> _apps;
  int _version = 0;
  StrideSubscription _file_subscription;
};
