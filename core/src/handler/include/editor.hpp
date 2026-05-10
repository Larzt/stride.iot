#pragma once

#include "handler.hpp"
#include "blackboard.hpp"
#include "stride_logger.hpp"


class Editor : public Handler
{
public:
  Editor();
  const std::vector<httpd_uri_t *> &uris() const override;

private:
  static esp_err_t handler(httpd_req_t *req);

  httpd_uri_t _editor_uri;
  std::vector<httpd_uri_t *> _uris;
};
