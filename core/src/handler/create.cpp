#include "create.hpp"

Create::Create()
{
  _create_uri = {
      .uri = "/create",
      .method = HTTP_POST,
      .handler = &Create::handler,
      .user_ctx = this};

  _uris = {&_create_uri};
}

const std::vector<httpd_uri_t *> &Create::uris() const
{
  return _uris;
}

esp_err_t Create::handler(httpd_req_t *req)
{
    char query[128] = {0};
    char file[64] = {0};

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK)
    {
        httpd_query_key_value(query, "file", file, sizeof(file));
    }

    if (strlen(file) == 0)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing file name");
        return ESP_FAIL;
    }

    std::string path = Blackboard::MountPoint + "/" + std::string(file);

    FILE *f = fopen(path.c_str(), "w");

    if (!f)
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Cannot create file");
        return ESP_FAIL;
    }

    fclose(f);

    Blackboard::FileListVersion = Blackboard::FileListVersion.get() + 1;

    httpd_resp_set_type(req, "text/plain");
    httpd_resp_sendstr(req, "OK");

    return ESP_OK;
}
