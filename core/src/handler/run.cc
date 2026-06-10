#include "run.hpp"

#include <string>
#include <cstring>

#include "app.hpp"
#include "card_task.hpp"
#include "interpreter.hpp"

static bool is_str_file(const std::string &fn)
{
  if (fn.length() < 5)
    return false;
  std::string ext = fn.substr(fn.find_last_of('.') == std::string::npos ? fn.length() : fn.find_last_of('.'));
  return ext == ".str" || ext == ".STR";
}

Run::Run()
{
  _run_uri = {
      .uri = "/run",
      .method = HTTP_POST,
      .handler = &Run::run_handler,
      .user_ctx = this};

  _status_uri = {
      .uri = "/running",
      .method = HTTP_GET,
      .handler = &Run::status_handler,
      .user_ctx = this};

  _uris = {&_run_uri, &_status_uri};
}

const std::vector<httpd_uri_t *> &Run::uris() const
{
  return _uris;
}

esp_err_t Run::run_handler(httpd_req_t *req)
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

  std::string fileName(file);
  if (!is_str_file(fileName))
  {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Only .str files can be executed");
    return ESP_FAIL;
  }

  AppDescriptor app;
  app.type = AppType::Script;
  app.name = fileName;
  app.path = "/" + fileName;

  Interpreter::Instance().stop_endless_loop();
  Blackboard::CurrentProgram = app;

  if (sdReadTaskHandle == NULL)
  {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Read task not ready");
    return ESP_FAIL;
  }

  xTaskNotifyGive(sdReadTaskHandle);

  httpd_resp_set_type(req, "text/plain");
  httpd_resp_sendstr(req, "OK");
  return ESP_OK;
}

esp_err_t Run::status_handler(httpd_req_t *req)
{
  std::string name = Blackboard::RunningProgramName.get();
  httpd_resp_set_type(req, "text/plain");
  httpd_resp_set_hdr(req, "Connection", "close");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  httpd_resp_sendstr(req, name.c_str());
  return ESP_OK;
}
