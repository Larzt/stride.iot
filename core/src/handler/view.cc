#include "view.hpp"

#include <fstream>
#include <sstream>

View::View()
{
  _view_uri = {
      .uri = "/view",
      .method = HTTP_GET,
      .handler = &View::handler,
      .user_ctx = this};

  _uris = {&_view_uri};
}

const std::vector<httpd_uri_t *> &View::uris() const
{
  return _uris;
}

esp_err_t View::handler(httpd_req_t *req)
{
  char query[128] = {0};
  char file[64] = {0};

  if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK)
  {
    httpd_query_key_value(query, "file", file, sizeof(file));
  }

  std::string path = Blackboard::MountPoint + std::string(file);

  std::ifstream in(path);
  std::stringstream buffer;

  if (in.is_open())
  {
    buffer << in.rdbuf();
    in.close();
  }

  std::string content = buffer.str();

  std::string html = R"rawliteral(
<div class="card">
    <h2>Vista: )rawliteral" + std::string(file) + R"rawliteral(</h2>
    <pre>)rawliteral"
    + content +
R"rawliteral(</pre>
</div>
)rawliteral";

  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req, html.c_str(), html.length());

  return ESP_OK;
}
