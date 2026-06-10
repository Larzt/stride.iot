#include "server.hpp"

#include "ping.hpp"
#include "root.hpp"
#include "browser.hpp"
#include "create.hpp"
#include "delete.hpp"
#include "editor.hpp"
#include "view.hpp"
#include "save.hpp"
#include "librarie.hpp"
#include "wifi.hpp"
#include "settings.hpp"
#include "run.hpp"
#include "access_point.hpp"

Server::Server() {}

Server::~Server()
{
  if (_server)
  {
    httpd_stop(_server);
  }
}

httpd_handle_t Server::start_server()
{
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = Blackboard::Port;
  config.max_req_hdr_len = Blackboard::HeaderLength;
  config.max_uri_handlers = Blackboard::MaxHandlers;
  config.lru_purge_enable = true;

  if (httpd_start(&_server, &config) != ESP_OK)
  {
    StrideLogger::Error(StrideSubsystem::Server, "Could not wake up the server");
    return nullptr;
  }

  load_handlers();
  register_handlers();

  StrideLogger::Log(StrideSubsystem::Server, "Server listening in port: %d", Blackboard::Port);
  return _server;
}

void Server::add_handler(Handler *handler)
{
  _handlers.push_back(handler);
}

void Server::reset_handlers()
{
  StrideLogger::Warning(StrideSubsystem::Server, "Clearing server routes");

  for (auto *handler : _handlers)
  {
    for (auto *uri : handler->uris())
    {
      if (httpd_register_uri_handler(_server, uri) != ESP_OK)
      {
        StrideLogger::Error(StrideSubsystem::Server, "Error registering URI");
      }
    }
  }

  for (auto *handler : _handlers)
  {
    delete handler;
  }

  _handlers.clear();

  load_handlers();
  register_handlers();
}

void Server::load_handlers()
{

  this->add_handler(new Root());
  this->add_handler(new Browser());
  this->add_handler(new Create());
  this->add_handler(new Delete());
  this->add_handler(new Editor());
  this->add_handler(new View());
  this->add_handler(new Save());
  this->add_handler(new Librarie());
  this->add_handler(new Ping());
  this->add_handler(new Settings());
  this->add_handler(new Run());
  if (Blackboard::CurrentServerMode.get() == ServerMode::Developer)
  {

    this->add_handler(new Wifi());
    this->add_handler(new AccessPoint());

  }
}

void Server::register_handlers()
{
  for (auto *handler : _handlers)
  {
    for (auto *uri : handler->uris())
    {
      if (httpd_register_uri_handler(_server, uri) != ESP_OK)
      {
        StrideLogger::Error(StrideSubsystem::Server, "Error registering URI");
      }
    }
  }
}
