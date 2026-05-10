#pragma once

#include "esp_http_server.h"
#include <vector>

class Handler
{
public:
  virtual ~Handler() = default;

  virtual const std::vector<httpd_uri_t*> &uris() const = 0;

  virtual bool enabled_in_ap_mode() const { return true; }
  virtual bool enabled_in_sta_mode() const { return true; }
};
