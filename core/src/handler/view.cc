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
  char offsetStr[16] = {0};

  size_t offset = 0;

  if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK)
  {
    httpd_query_key_value(query, "file", file, sizeof(file));
    httpd_query_key_value(query, "offset", offsetStr, sizeof(offsetStr));
    offset = atoi(offsetStr);
  }

  std::string path = Blackboard::MountPoint + "/" + std::string(file);

  FILE *f = fopen(path.c_str(), "r");

  std::string content = "";

  if (f)
  {
    const size_t CHUNK_SIZE = 2048;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);

    if (offset < (size_t)size)
    {
      fseek(f, offset, SEEK_SET);

      size_t toRead = CHUNK_SIZE;
      if (offset + CHUNK_SIZE > (size_t)size)
        toRead = size - offset;

      content.resize(toRead);
      fread(content.data(), 1, toRead, f);
    }
    else
    {
      content = "Offset fuera de rango";
    }

    fclose(f);
  }
  else
  {
    content = "File not found";
  }

  size_t nextOffset = offset + content.size();

  std::string html =
      "<div class='card'>"
      "<h2>Viewer: " +
      std::string(file) + "</h2>"
                          "<pre style='background:#111;color:#0f0;padding:10px;overflow:auto;'>" +
      content +
      "</pre>"
      "<div style='margin-top:10px;'>"
      "<button onclick=\"loadPage('/view?file=" +
      std::string(file) +
      "&offset=" + std::to_string(offset > 2048 ? offset - 2048 : 0) +
      "')\">⬅ Prev</button> "
      "<button onclick=\"loadPage('/view?file=" +
      std::string(file) +
      "&offset=" + std::to_string(nextOffset) +
      "')\">Next ➡</button>"
      "</div>"
      "</div>";

  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req, html.c_str(), html.length());

  return ESP_OK;
}
