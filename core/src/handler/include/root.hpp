#pragma once

#include "handler.hpp"

class Root : public Handler
{
public:
  Root();

  const std::vector<httpd_uri_t *> &uris() const override;

private:
  static esp_err_t handler(httpd_req_t *req);

  httpd_uri_t _root_uri;
  std::vector<httpd_uri_t *> _uris;
};
