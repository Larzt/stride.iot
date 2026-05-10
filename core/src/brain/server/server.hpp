#pragma once
#include <vector>
#include "esp_http_server.h"

#include "network.hpp"
#include "handler.hpp"
#include "enums.hpp"


class Server
{

public:
  Server();
  ~Server();

  httpd_handle_t start_server();

  void add_handler(Handler *handler);
  void reset_handlers();

private:
  httpd_handle_t _server;
  std::vector<Handler *> _handlers;

  void load_handlers();
  void register_handlers();
};
