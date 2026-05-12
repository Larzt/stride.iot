#pragma once

#include <unordered_map>
#include <typeindex>
#include <string>
#include <stdexcept>

class StrideLocator
{
public:
  template <typename T>
  static void Register(T *service)
  {
    if (service == nullptr)
      return;
    services[std::type_index(typeid(T))] = static_cast<void *>(service);
  }

  template <typename T>
  static T *Get()
  {
    auto it = services.find(std::type_index(typeid(T)));
    if (it != services.end())
    {
      return static_cast<T *>(it->second);
    }

    printf("Error: Servicio %s no registrado.\n", typeid(T).name());
    return nullptr;
  }

  template <typename T>
  static void Unregister()
  {
    services.erase(std::type_index(typeid(T)));
  }

private:
  inline static std::unordered_map<std::type_index, void *> services;
};
