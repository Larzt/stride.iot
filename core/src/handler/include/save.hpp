#pragma once

#include "handler.hpp"
#include "blackboard.hpp"
#include "stride_logger.hpp"

class Save : public Handler
{
public:
  Save();
  const std::vector<httpd_uri_t *> &uris() const override;

private:
  static esp_err_t handler(httpd_req_t *req);

  httpd_uri_t _save_uri;
  std::vector<httpd_uri_t *> _uris;
};
