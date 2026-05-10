#include "server.hpp"

#include "ping.hpp"
#include "root.hpp"
#include "browser.hpp"
#include "editor.hpp"
#include "view.hpp"
#include "save.hpp"
#include "librarie.hpp"

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
  this->add_handler(new Editor());
  this->add_handler(new View());
  this->add_handler(new Save());
  this->add_handler(new Librarie());
  if (Blackboard::CurrentServerMode.get() == ServerMode::Developer)
  {
    // The first time you turn on the device, it will be in developer mode; once you've set up the Wi-Fi, switch to user mode so that this handler cannot be accessed.
    // this->add_handler(new WifiConfigHandler());

    this->add_handler(new Ping());

    // this->add_handler(new StatusHandler());
    // this->add_handler(new ConfigHandler());
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
