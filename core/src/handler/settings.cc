#include "settings.hpp"
#include <string>

Settings::Settings()
{
  _uri.uri = "/settings";
  _uri.method = HTTP_GET;
  _uri.handler = &Settings::handler;
  _uri.user_ctx = this;

  _uris = {&_uri};
}

const std::vector<httpd_uri_t *> &Settings::uris() const
{
  return _uris;
}

esp_err_t Settings::handler(httpd_req_t *req)
{
  NetworkMode netMode = Blackboard::CurrentNetworkMode.get();
  ServerMode srvMode = Blackboard::CurrentServerMode.get();
  bool isDev = (srvMode == ServerMode::Developer);

  std::string ssid;
  std::string ip;
  std::string modeLabel;
  std::string modeColor;

  if (netMode == NetworkMode::Access)
  {
    ssid = Blackboard::AccessSSID;
    ip = Blackboard::LocalIpAddress.get();
    modeLabel = "Punto de acceso";
    modeColor = "#f59e0b";
  }
  else
  {
    std::string pass;
    Network::load_net_credentials(ssid, pass);
    ip = Blackboard::WifiIpAddress.get();
    modeLabel = "Estaci&oacute;n";
    modeColor = "#10b981";
  }

  std::string srvLabel = isDev ? "Desarrollo" : "Producci&oacute;n";
  std::string srvColor = isDev ? "#2563eb" : "#64748b";

  std::string html =
      "<div class='card'>"
      "<div class='page-hdr'>"
      "<h2 style='font-size:1rem;font-weight:600;'>Ajustes</h2>"
      "</div>"

      "<div style='display:flex;flex-direction:column;gap:.75rem;margin-top:.25rem;'>"

      "<div style='display:flex;align-items:center;justify-content:space-between;"
      "padding:.75rem;border:1px solid var(--border);border-radius:var(--r);'>"
      "<span style='font-size:.82rem;color:var(--muted);'>Modo de red</span>"
      "<span style='font-size:.82rem;font-weight:600;color:" +
      modeColor + ";'>" + modeLabel + "</span>"
                                      "</div>"

                                      "<div style='display:flex;align-items:center;justify-content:space-between;"
                                      "padding:.75rem;border:1px solid var(--border);border-radius:var(--r);'>"
                                      "<span style='font-size:.82rem;color:var(--muted);'>Red</span>"
                                      "<span style='font-size:.82rem;font-weight:600;'>" +
      ssid + "</span>"
             "</div>"

             "<div style='display:flex;align-items:center;justify-content:space-between;"
             "padding:.75rem;border:1px solid var(--border);border-radius:var(--r);'>"
             "<span style='font-size:.82rem;color:var(--muted);'>Direcci&oacute;n IP</span>"
             "<span style='font-size:.82rem;font-family:monospace;font-weight:600;'>" +
      ip + "</span>"
           "</div>"

           "<div style='display:flex;align-items:center;justify-content:space-between;"
           "padding:.75rem;border:1px solid var(--border);border-radius:var(--r);'>"
           "<span style='font-size:.82rem;color:var(--muted);'>Modo servidor</span>"
           "<span style='font-size:.82rem;font-weight:600;color:" +
      srvColor + ";'>" + srvLabel + "</span>"
                                    "</div>"

                                    "</div>";

  if (isDev)
  {
    html +=
        "<div style='margin-top:1.25rem;padding:.75rem 1rem;"
        "background:#eff6ff;border:1px solid #bfdbfe;border-radius:var(--r);'>"
        "<p style='margin:0 0 .75rem;font-size:.8rem;color:#1e40af;'>"
        "Modo desarrollo activo. Puedes configurar las redes Wi-Fi."
        "</p>"
        "<div style='display:flex;flex-wrap:wrap;gap:.5rem;justify-content:flex-end;'>"
        "<button class='btn' style='white-space:nowrap;' onclick=\"loadPage('/ap')\">"
        "Configurar punto de acceso"
        "</button>"
        "<button class='btn btn-primary' style='white-space:nowrap;' onclick=\"loadPage('/wifi')\">"
        "Configurar Wi-Fi"
        "</button>"
        "</div>"
        "</div>";
  }

  html += "</div>";

  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req, html.c_str(), html.length());
  return ESP_OK;
}
