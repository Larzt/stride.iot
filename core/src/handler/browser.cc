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
  int fileCount = 0;

  {
    DIR *d2 = opendir(Blackboard::MountPoint.c_str());
    if (d2) {
      struct dirent *e2;
      while ((e2 = readdir(d2)) != NULL) {
        std::string fn = e2->d_name;
        if (fn == "." || fn == ".." || fn.length() < 4 || fn.find('.') == std::string::npos) continue;
        std::string ex = fn.substr(fn.find_last_of('.'));
        if (ex == ".log" || ex == ".LOG" || ex == ".str" || ex == ".STR") fileCount++;
      }
      closedir(d2);
    }
  }

  html += "<div class=\"card\">";
  html += "<div class=\"page-hdr\">";
  html += "<div>";
  html += "<h2 style=\"font-size:1rem;font-weight:600;\">Archivos SD</h2>";
  html += "<p style=\"font-size:.78rem;color:var(--muted);margin-top:.2rem;\">" + std::to_string(fileCount) + " archivo" + (fileCount != 1 ? "s" : "") + "</p>";
  html += "</div>";
  html += "<button class=\"btn btn-primary\" onclick=\"createFile()\">+ Nuevo</button>";
  html += "</div>";

  html += R"rawliteral(
    <table class="ftable">
        <thead>
            <tr>
                <th>Nombre</th>
                <th style="text-align:right;">Acciones</th>
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

    if (fileName.length() < 4)
      continue;

    if (fileName.find('.') == std::string::npos)
      continue;

    std::string ext = fileName.substr(fileName.find_last_of('.'));
    if (ext != ".log" && ext != ".LOG" && ext != ".str" && ext != ".STR")
      continue;

    hasFiles = true;

    html += "<tr>";
    html += "<td><span class=\"fname\">&#128196; " + fileName + "</span></td>";
    html += "<td><div class=\"actions\">";
    html += "<button class=\"btn-icon\" title=\"Editar\" onclick=\"editFile('" + fileName + "')\">&#9998;</button>";
    html += "<button class=\"btn-icon\" title=\"Ver\" onclick=\"viewFile('" + fileName + "')\">&#128065;</button>";
    html += "<button class=\"btn-icon\" title=\"Eliminar\" onclick=\"deleteFile('" + fileName + "')\" style=\"color:#ef4444\">&#128465;</button>";
    html += "</div></td>";
    html += "</tr>";
  }

  closedir(dir);

  if (!hasFiles)
  {
    html += "<tr><td colspan='2'>"
            "<div class=\"empty\"><div class=\"empty-icon\">&#128193;</div>"
            "<p>No hay archivos en la SD</p></div>"
            "</td></tr>";
  }

  html += "</tbody></table></div>";

  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req, html.c_str(), html.length());

  return ESP_OK;
}
