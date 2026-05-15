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

  size_t prevOffset = offset > 2048 ? offset - 2048 : 0;
  std::string fileEnc = std::string(file);

  std::string html =
      "<div class='card'>"
      "<div class='page-hdr'>"
      "<div class='page-hdr-left'>"
      "<button class='btn' onclick=\"loadPage('/browser')\">&#8592; Volver</button>"
      "<h2 style='font-size:.95rem;font-weight:600;color:var(--muted)'>" + fileEnc + "</h2>"
      "</div>"
      "</div>"
      "<div class='log-view'>" + content + "</div>"
      "<div class='pagination'>"
      "<button class='btn' " + (offset == 0 ? "disabled style='opacity:.4;cursor:default'" : "") +
      " onclick=\"loadPage('/view?file=" + fileEnc + "&offset=" + std::to_string(prevOffset) + "')\">&#8592; Anterior</button>"
      "<button class='btn' onclick=\"loadPage('/view?file=" + fileEnc + "&offset=" + std::to_string(nextOffset) + "')\">Siguiente &#8594;</button>"
      "<span class='page-info'>Offset: " + std::to_string(offset) + "</span>"
      "</div>"
      "</div>";

  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req, html.c_str(), html.length());

  return ESP_OK;
}
