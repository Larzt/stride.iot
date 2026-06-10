#pragma once

#include "handler.hpp"
#include "blackboard.hpp"
#include "network.hpp"
#include "stride_locator.hpp"
#include "stride_logger.hpp"

class AccessPoint : public Handler
{
public:
  AccessPoint();
  const std::vector<httpd_uri_t *> &uris() const override;

private:
  static esp_err_t get_handler(httpd_req_t *req);
  static esp_err_t post_handler(httpd_req_t *req);

  httpd_uri_t _get_uri;
  httpd_uri_t _post_uri;
  std::vector<httpd_uri_t *> _uris;
};
