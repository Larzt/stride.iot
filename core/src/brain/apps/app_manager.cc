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
  std::vector<AppDescriptor> found;

  if (!Blackboard::SdCardMounted.get())
  {
    if (!_scripts.empty())
    {
      _scripts.clear();
      rebuild_apps();
      _version++;
      StrideLogger::Log(StrideSubsystem::Card, "AppManager: scripts cleared (no SD card)");
    }
    return;
  }

  DIR *dir = opendir(Blackboard::MountPoint.c_str());
  if (!dir)
  {
    if (!_scripts.empty())
    {
      _scripts.clear();
      rebuild_apps();
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

  bool changed = (found.size() != _scripts.size());
  if (!changed)
  {
    for (size_t i = 0; i < found.size(); i++)
    {
      if (found[i].name != _scripts[i].name)
      {
        changed = true;
        break;
      }
    }
  }

  if (changed)
  {
    _scripts = std::move(found);
    rebuild_apps();
    _version++;
    StrideLogger::Log(StrideSubsystem::Card, "AppManager: %d apps (v%d)", (int)_apps.size(), _version);
  }
}

void AppManager::register_app(const AppDescriptor &app)
{
  for (const auto &existing : _builtins)
  {
    if (existing == app)
      return;
  }

  _builtins.push_back(app);
  rebuild_apps();
  _version++;
  StrideLogger::Log(StrideSubsystem::Card, "AppManager: registered '%s' (v%d)", app.name.c_str(), _version);
}

void AppManager::rebuild_apps()
{
  _apps.clear();
  _apps.reserve(_builtins.size() + _scripts.size());
  _apps.insert(_apps.end(), _builtins.begin(), _builtins.end());
  _apps.insert(_apps.end(), _scripts.begin(), _scripts.end());
}
