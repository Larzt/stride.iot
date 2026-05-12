#include "wifi.hpp"
#include <sstream>
#include "esp_log.h"

Wifi::Wifi()
{
  _get_uri.uri = "/wifi";
  _get_uri.method = HTTP_GET;
  _get_uri.handler = &Wifi::get_handler;
  _get_uri.user_ctx = this;

  _post_uri.uri = "/wifi";
  _post_uri.method = HTTP_POST;
  _post_uri.handler = &Wifi::post_handler;
  _post_uri.user_ctx = this;

  _uris = {&_get_uri, &_post_uri};
}

const std::vector<httpd_uri_t *> &Wifi::uris() const
{
  return _uris;
}

esp_err_t Wifi::get_handler(httpd_req_t *req)
{
  class Network *net = StrideLocator::Get<class Network>();

  if (!net)
  {
    StrideLogger::Log(StrideSubsystem::Network, "Network in Wifi handler is null");
    return ESP_FAIL;
  }

  // Wifi *self = static_cast<Wifi *>(req->user_ctx);

  std::string ssid, password;
  net->load_net_credentials(ssid, password);

  std::stringstream html;
  html << R"rawliteral(
    <style>
        :root {
            --primary: #007bff;
            --bg-card: #ffffff;
            --text-main: #333;
            --input-border: #ddd;
        }
        .card {
            background: var(--bg-card);
            padding: 20px;
            border-radius: 12px;
            box-shadow: 0 4px 15px rgba(0,0,0,0.1);
            max-width: 400px;
            margin: 20px auto;
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
        }
        h2 { margin-top: 0; color: var(--text-main); font-size: 1.5rem; }
        .doc-subtitle { color: #666; font-size: 0.9rem; margin-bottom: 20px; }

        label {
            display: block;
            font-weight: 600;
            margin-bottom: 5px;
            font-size: 0.85rem;
            color: #555;
        }
        input[type="text"], input[type="password"] {
            width: 100%;
            padding: 10px;
            margin-bottom: 15px;
            border: 1px solid var(--input-border);
            border-radius: 6px;
            box-sizing: border-box; /* Crucial para que el padding no rompa el ancho */
            transition: border-color 0.2s;
        }
        input:focus {
            outline: none;
            border-color: var(--primary);
            box-shadow: 0 0 0 3px rgba(0,123,255,0.1);
        }
        .nav-item {
            cursor: pointer;
            font-weight: 600;
            padding: 12px;
            border-radius: 6px;
            transition: opacity 0.2s, transform 0.1s;
        }
        .nav-item:hover { opacity: 0.9; }
        .nav-item:active { transform: scale(0.98); }
    </style>

    <div class="card">
        <h2>Configuración de Red</h2>
        <p class="doc-subtitle">Introduce las credenciales para el modo Estación.</p>

        <form id="wifi-form">
            <label>SSID de la Red</label>
            <input type="text" id="ssid" name="ssid" style="width:100%; padding:8px; margin:10px 0;" value=")rawliteral"
       << ssid << R"rawliteral(">

            <label>Contraseña</label>
            <input type="password" id="pass" name="password" style="width:100%; padding:8px; margin:10px 0;" value=")rawliteral"
       << password << R"rawliteral(">

            <button type="button" class="nav-item" style="background:var(--primary); color:white; border:none; width:100%; margin-top:10px;"
                    onclick="sendWifiConfig()">
                Guardar y Conectar
            </button>
        </form>
    </div>

    <script>
    function sendWifiConfig() {
        const ssid = document.getElementById('ssid').value;
        const pass = document.getElementById('pass').value;
        const body = "ssid=" + encodeURIComponent(ssid) + "&password=" + encodeURIComponent(pass);

        fetch('/wifi', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: body
        }).then(res => {
            if(res.ok) {
                // Redirigir al home tras un breve delay para que el usuario vea el éxito
                alert("Configuración guardada. Conectando...");
                window.location.href = "/";
            } else {
                alert("Error al guardar.");
            }
        });
    }
    </script>
    )rawliteral";

  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, html.str().c_str(), HTTPD_RESP_USE_STRLEN);
}

esp_err_t Wifi::post_handler(httpd_req_t *req)
{
  class Network *net = StrideLocator::Get<class Network>();

  // Wifi *self = static_cast<Wifi *>(req->user_ctx);
  char buf[256];
  int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
  if (ret <= 0)
    return ESP_FAIL;
  buf[ret] = '\0';

  std::string body(buf);

  auto parse_val = [&](std::string key)
  {
    size_t pos = body.find(key + "=");
    if (pos == std::string::npos)
      return std::string("");
    size_t start = pos + key.length() + 1;
    size_t end = body.find("&", start);
    return body.substr(start, end - start);
  };

  std::string ssid = parse_val("ssid");
  std::string password = parse_val("password");

  net->save_net_credentials(ssid, password);

  esp_wifi_disconnect();
  esp_wifi_connect();

  httpd_resp_sendstr(req, "OK");
  return ESP_OK;
}
