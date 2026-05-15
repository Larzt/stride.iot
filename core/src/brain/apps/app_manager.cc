#include "app_manager.hpp"
#include "stride_logger.hpp"

AppManager &AppManager::Instance()
{
  static AppManager instance;
  return instance;
}

AppManager::AppManager()
{
  StrideLocator::Register<AppManager>(this);

  _file_subscription = Blackboard::FileListVersion.subscribe([this](int)
                                                             { scan(); });
}

void AppManager::scan()
{
  if (!Blackboard::SdCardMounted.get())
  {
    if (!_apps.empty())
    {
      _apps.clear();
      _version++;
      StrideLogger::Log(StrideSubsystem::Card, "AppManager: list cleared (no SD card)");
    }
    return;
  }

  std::vector<AppDescriptor> found;

  DIR *dir = opendir(Blackboard::MountPoint.c_str());
  if (!dir)
  {
    if (!_apps.empty())
    {
      _apps.clear();
      _version++;
    }
    return;
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL)
  {
    std::string name = std::string(entry->d_name);
    if (name.length() < 5 || name.find('.') == std::string::npos)
      continue;

    std::string ext = name.substr(name.find_last_of('.'));
    if (ext != ".str" && ext != ".STR" && ext != ".log" && ext != ".LOG")
      continue;

    AppDescriptor app;
    app.name = name;
    app.path = "/" + name;
    app.type = AppType::Script;
    found.push_back(app);
  }
  closedir(dir);

  bool changed = (found.size() != _apps.size());
  if (!changed)
  {
    for (size_t i = 0; i < found.size(); i++)
    {
      if (found[i].name != _apps[i].name)
      {
        changed = true;
        break;
      }
    }
  }

  if (changed)
  {
    _apps = std::move(found);
    _version++;
    StrideLogger::Log(StrideSubsystem::Card, "AppManager: %d apps (v%d)", (int)_apps.size(), _version);
  }
}
