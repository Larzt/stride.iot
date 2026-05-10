#pragma once
#include "stride_subscription.hpp"

#include <vector>
#include <functional>

template <typename T>
class StrideObservable
{
public:
  using Callback = std::function<void(const T &)>;

  explicit StrideObservable(T initial_value) : _value(initial_value) {}

  StrideSubscription subscribe(Callback callback)
  {
    _callbacks.push_back(callback);
    auto it = std::prev(_callbacks.end());

    return StrideSubscription([this, it]()
    {
      _callbacks.erase(it);
    });
  }

  void set(const T &new_value)
  {
    if (_value != new_value)
    {
      _value = new_value;
      notify();
    }
  }

  T get() const { return _value; }

  StrideObservable &operator=(const T &new_value)
  {
    set(new_value);
    return *this;
  }

private:
  T _value;
  std::vector<Callback> _callbacks;

  void notify()
  {
    for (auto &cb : _callbacks)
    {
      cb(_value);
    }
  }
};
