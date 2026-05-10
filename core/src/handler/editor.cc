#include "editor.hpp"

#include <fstream>
#include <sstream>

Editor::Editor()
{
  _editor_uri = {
      .uri = "/editor",
      .method = HTTP_GET,
      .handler = &Editor::handler,
      .user_ctx = this};

  _uris = {&_editor_uri};
}

const std::vector<httpd_uri_t *> &Editor::uris() const
{
  return _uris;
}

esp_err_t Editor::handler(httpd_req_t *req)
{
  char query[128] = {0};
  char file[64] = {0};

  if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK)
  {
    httpd_query_key_value(query, "file", file, sizeof(file));
  }

  std::string path = Blackboard::MountPoint + "/" + std::string(file);

  FILE *f = fopen(path.c_str(), "r");

  std::string content;

  if (f)
  {
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    content.resize(size);
    fread(content.data(), 1, size, f);

    fclose(f);
  }
  else
  {
    StrideLogger::Warning(StrideSubsystem::Card, "File not found: %s", path.c_str());
  }

  std::string html =
    "<div class='card'>"
    "<h2>Editor: " + std::string(file) + "</h2>"
    "<textarea id='editor' style='width:100%; height:300px;'>" +
    content +
    "</textarea>"
    "<br><br>"
    "<button onclick=\"saveFile('" + std::string(file) + "')\">Guardar</button>"
    "</div>";

  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req, html.c_str(), html.length());

  return ESP_OK;
}
