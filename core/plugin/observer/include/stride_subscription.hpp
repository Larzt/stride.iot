#pragma once
#include <functional>
#include <vector>

class StrideSubscription
{
public:
  StrideSubscription() = default;

  StrideSubscription(std::function<void()> unsubscribe)
    : _unsubscribe(unsubscribe) {}

  void unsubscribe()
  {
    if (_unsubscribe)
    {
      _unsubscribe();
      _unsubscribe = nullptr;
    }
  }

private:
  std::function<void()> _unsubscribe;
};
