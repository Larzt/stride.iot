#pragma once

#include "handler.hpp"
#include "blackboard.hpp"

class View : public Handler
{
public:
  View();
  const std::vector<httpd_uri_t *> &uris() const override;

private:
  static esp_err_t handler(httpd_req_t *req);

  httpd_uri_t _view_uri;
  std::vector<httpd_uri_t *> _uris;
};
