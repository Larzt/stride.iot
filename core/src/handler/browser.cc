#include "browser.hpp"

#include <dirent.h>
#include <sys/stat.h>
#include <string>

Browser::Browser()
{
  _browser_uri = {
      .uri = "/browser",
      .method = HTTP_GET,
      .handler = &Browser::handler,
      .user_ctx = this};

  _uris = {&_browser_uri};
}

const std::vector<httpd_uri_t *> &Browser::uris() const
{
  return _uris;
}

esp_err_t Browser::handler(httpd_req_t *req)
{
  DIR *dir = opendir(Blackboard::MountPoint.c_str());
  if (!dir)
  {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No se pudo leer la SD");
    return ESP_FAIL;
  }

  std::string html;

  html += R"rawliteral(
<div class="card">
    <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:1rem;">
        <h2>📁 Explorador SD</h2>
        <button onclick="createFile()">+ Nuevo</button>
    </div>

    <table style="width:100%; border-collapse:collapse;">
        <thead>
            <tr>
                <th style="text-align:left;">Archivo</th>
                <th>Acciones</th>
            </tr>
        </thead>
        <tbody>
)rawliteral";

  struct dirent *entry;
  bool hasFiles = false;

  while ((entry = readdir(dir)) != NULL)
  {
    std::string fileName = entry->d_name;

    if (fileName == "." || fileName == "..")
      continue;

    // Validar extensión (evitar crash si nombre corto)
    if (fileName.length() < 4)
      continue;

    std::string ext = fileName.substr(fileName.length() - 4);
    if (ext != ".log" && ext != ".LOG" && ext != ".str" && ext != ".STR")
      continue;

    hasFiles = true;

    html += "<tr>";
    html += "<td>📄 " + fileName + "</td>";
    html += "<td>";
    html += "<button onclick=\"editFile('" + fileName + "')\">Editar</button>";
    html += "<button onclick=\"viewFile('" + fileName + "')\">Ver</button>";
    html += "</td>";
    html += "</tr>";
  }

  closedir(dir);

  if (!hasFiles)
  {
    html += "<tr><td colspan='2'>SD vacía</td></tr>";
  }

  html += R"rawliteral(
        </tbody>
    </table>
</div>
)rawliteral";

  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req, html.c_str(), html.length());

  return ESP_OK;
}
