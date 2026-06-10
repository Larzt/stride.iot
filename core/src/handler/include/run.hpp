#pragma once

#include "handler.hpp"
#include "blackboard.hpp"

class Run : public Handler
{
public:
  Run();
  const std::vector<httpd_uri_t *> &uris() const override;

private:
  static esp_err_t run_handler(httpd_req_t *req);
  static esp_err_t status_handler(httpd_req_t *req);

  httpd_uri_t _run_uri;
  httpd_uri_t _status_uri;
  std::vector<httpd_uri_t *> _uris;
};
