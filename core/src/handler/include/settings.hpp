#pragma once

#include "handler.hpp"
#include "blackboard.hpp"
#include "stride_locator.hpp"
#include "stride_logger.hpp"
#include "network.hpp"
#include "enums.hpp"

class Settings : public Handler
{
public:
  Settings();
  const std::vector<httpd_uri_t *> &uris() const override;

private:
  httpd_uri_t _uri{};
  std::vector<httpd_uri_t *> _uris;

  static esp_err_t handler(httpd_req_t *req);
};
