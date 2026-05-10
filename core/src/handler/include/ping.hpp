#pragma once

#include "handler.hpp"
#include "stride_led.hpp"
#include "blackboard.hpp"

class Ping : public Handler
{
public:
  Ping();

  const std::vector<httpd_uri_t*> &uris() const override;

private:
  static esp_err_t handler(httpd_req_t *req);

  StrideLed _led;
  httpd_uri_t _ping_uri;
  std::vector<httpd_uri_t*> _uris;
};
