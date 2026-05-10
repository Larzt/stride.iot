#include "ping.hpp"

#include <string>

Ping::Ping() : _led(Blackboard::PingLed)
{
    _ping_uri = {
        .uri = "/ping",
        .method = HTTP_GET,
        .handler = &Ping::handler,
        .user_ctx = this};

    _uris = {&_ping_uri};
    _led.start();
}

const std::vector<httpd_uri_t *> &Ping::uris() const
{
    return _uris;
}

esp_err_t Ping::handler(httpd_req_t *req)
{
    const std::string kENDL = "\n";
    Ping *self = static_cast<Ping *>(req->user_ctx);

    self->_led.toggle();

    std::string resp = "{" + kENDL;
    resp += "  \"status\": \"pong\"," + kENDL;
    resp += "  \"led_state\": " + std::to_string(self->_led.get()) + kENDL;
    resp += "}";

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp.c_str(), resp.length());
    return ESP_OK;
}
