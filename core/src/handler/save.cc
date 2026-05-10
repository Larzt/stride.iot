#include "save.hpp"

#include <fstream>
#include <cstring>

Save::Save()
{
  _save_uri = {
      .uri = "/save",
      .method = HTTP_POST,
      .handler = &Save::handler,
      .user_ctx = this};

  _uris = {&_save_uri};
}

const std::vector<httpd_uri_t *> &Save::uris() const
{
  return _uris;
}

esp_err_t Save::handler(httpd_req_t *req)
{
  char query[128] = {0};
  char file[64] = {0};

  if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK)
  {
    httpd_query_key_value(query, "file", file, sizeof(file));
  }

  if (strlen(file) == 0 || strstr(file, ".."))
  {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid filename");
    return ESP_FAIL;
  }

  std::string path = Blackboard::MountPoint + "/" + std::string(file);

  FILE *f = fopen(path.c_str(), "w");
  if (!f)
  {
    StrideLogger::Error(StrideSubsystem::Card, "fopen failed: %s", strerror(errno));
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Cannot open file");
    return ESP_FAIL;
  }

  int remaining = req->content_len;
  char buffer[512];

  while (remaining > 0)
  {
    int to_read = remaining > sizeof(buffer) ? sizeof(buffer) : remaining;

    int received = httpd_req_recv(req, buffer, to_read);

    if (received <= 0)
    {
      fclose(f);
      StrideLogger::Error(StrideSubsystem::Card, "Receive error in save handler");
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Receive error");
      return ESP_FAIL;
    }

    fwrite(buffer, 1, received, f);
    remaining -= received;
  }

  fclose(f);
  httpd_resp_set_type(req, "text/plain");
  httpd_resp_sendstr(req, "OK");

  return ESP_OK;
}
