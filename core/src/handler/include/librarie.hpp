#pragma once

#include "handler.hpp"

class Librarie : public Handler
{
public:
  Librarie();
  const std::vector<httpd_uri_t *> &uris() const override;

private:
  static esp_err_t handler(httpd_req_t *req);

  httpd_uri_t _librarie_uri;
  std::vector<httpd_uri_t *> _uris;
};
