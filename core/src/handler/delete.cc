#include "delete.hpp"

Delete::Delete()
{
  _delete_uri = {
      .uri = "/delete",
      .method = HTTP_POST,
      .handler = &Delete::handler,
      .user_ctx = this};

  _uris = {&_delete_uri};
}

const std::vector<httpd_uri_t *> &Delete::uris() const
{
  return _uris;
}

esp_err_t Delete::handler(httpd_req_t *req)
{
  char query[128] = {0};
  char file[64] = {0};

  if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK)
  {
    httpd_query_key_value(query, "file", file, sizeof(file));
  }

  if (strlen(file) == 0)
  {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing file");
    return ESP_FAIL;
  }

  std::string path = Blackboard::MountPoint + "/" + std::string(file);

  if (remove(path.c_str()) != 0)
  {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Delete failed");
    return ESP_FAIL;
  }

  Blackboard::FileListVersion = Blackboard::FileListVersion.get() + 1;

  httpd_resp_set_type(req, "text/plain");
  httpd_resp_sendstr(req, "OK");

  return ESP_OK;
}
