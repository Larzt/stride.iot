#include "access_point.hpp"
#include <sstream>
#include <cstring>

AccessPoint::AccessPoint()
{
  _get_uri.uri = "/ap";
  _get_uri.method = HTTP_GET;
  _get_uri.handler = &AccessPoint::get_handler;
  _get_uri.user_ctx = this;

  _post_uri.uri = "/ap";
  _post_uri.method = HTTP_POST;
  _post_uri.handler = &AccessPoint::post_handler;
  _post_uri.user_ctx = this;

  _uris = {&_get_uri, &_post_uri};
}

const std::vector<httpd_uri_t *> &AccessPoint::uris() const
{
  return _uris;
}

static std::string url_decode(const std::string &in)
{
  std::string out;
  out.reserve(in.size());
  for (size_t i = 0; i < in.size(); ++i)
  {
    if (in[i] == '+')
    {
      out += ' ';
    }
    else if (in[i] == '%' && i + 2 < in.size())
    {
      auto hex = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return 10 + c - 'a';
        if (c >= 'A' && c <= 'F') return 10 + c - 'A';
        return -1;
      };
      int h = hex(in[i + 1]);
      int l = hex(in[i + 2]);
      if (h >= 0 && l >= 0)
      {
        out += static_cast<char>((h << 4) | l);
        i += 2;
      }
      else
      {
        out += in[i];
      }
    }
    else
    {
      out += in[i];
    }
  }
  return out;
}

esp_err_t AccessPoint::get_handler(httpd_req_t *req)
{
  std::string ssid, password;
  if (!Network::load_ap_credentials(ssid, password))
  {
    ssid = Blackboard::AccessSSID;
    password = Blackboard::AccessPASS;
  }

  std::stringstream html;
  html << R"rawliteral(
    <div class="card" style="max-width:480px;">
      <div class="page-hdr">
        <h2 style="font-size:1rem;font-weight:600;">Punto de acceso</h2>
      </div>
      <p style="font-size:.8rem;color:var(--muted);margin-bottom:1rem;">
        Cambia el SSID y la contrase&ntilde;a de la red Wi-Fi propia del dispositivo.
        La contrase&ntilde;a debe tener al menos 8 caracteres (WPA2).
      </p>

      <form id="ap-form" onsubmit="return false;">
        <div class="form-group">
          <label class="form-label">SSID</label>
          <input class="form-input" type="text" id="ap-ssid" maxlength="31" value=")rawliteral"
       << ssid << R"rawliteral(">
        </div>

        <div class="form-group">
          <label class="form-label">Contrase&ntilde;a</label>
          <input class="form-input" type="text" id="ap-pass" maxlength="63" value=")rawliteral"
       << password << R"rawliteral(">
        </div>

        <div style="display:flex;gap:.5rem;justify-content:flex-end;margin-top:1rem;">
          <button type="button" class="btn" onclick="loadPage('/settings')">Cancelar</button>
          <button type="button" class="btn btn-primary" onclick="sendAccessPointConfig()">Guardar</button>
        </div>
      </form>
    </div>

    <script>
    function sendAccessPointConfig() {
      var ssid = document.getElementById('ap-ssid').value.trim();
      var pass = document.getElementById('ap-pass').value;
      if (!ssid) { toast('SSID requerido', true); return; }
      if (pass.length > 0 && pass.length < 8) { toast('La contraseña debe tener al menos 8 caracteres', true); return; }

      var body = 'ssid=' + encodeURIComponent(ssid) + '&password=' + encodeURIComponent(pass);
      fetch('/ap', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: body
      })
      .then(function(r){ if(!r.ok) throw new Error(); return r.text(); })
      .then(function(){
        toast('Guardado. Reconecta a la nueva red.');
        setTimeout(function(){ loadPage('/settings'); }, 1200);
      })
      .catch(function(){ toast('Error al guardar', true); });
    }
    </script>
  )rawliteral";

  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, html.str().c_str(), HTTPD_RESP_USE_STRLEN);
}

esp_err_t AccessPoint::post_handler(httpd_req_t *req)
{
  char buf[256] = {0};
  int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
  if (ret <= 0)
  {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Empty body");
    return ESP_FAIL;
  }
  buf[ret] = '\0';

  std::string body(buf);
  auto parse_val = [&](const std::string &key) -> std::string {
    size_t pos = body.find(key + "=");
    if (pos == std::string::npos)
      return "";
    size_t start = pos + key.length() + 1;
    size_t end = body.find('&', start);
    return body.substr(start, end == std::string::npos ? std::string::npos : end - start);
  };

  std::string ssid = url_decode(parse_val("ssid"));
  std::string password = url_decode(parse_val("password"));

  if (ssid.empty() || ssid.length() > 31)
  {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid SSID");
    return ESP_FAIL;
  }

  if (!password.empty() && (password.length() < 8 || password.length() > 63))
  {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Password must be 8..63 chars or empty (open)");
    return ESP_FAIL;
  }

  Network::save_ap_credentials(ssid, password);

  class Network *net = StrideLocator::Get<class Network>();
  if (net)
  {
    net->apply_ap_credentials();
  }

  StrideLogger::Log(StrideSubsystem::Network, "AP credentials updated (ssid=%s)", ssid.c_str());

  httpd_resp_set_type(req, "text/plain");
  httpd_resp_sendstr(req, "OK");
  return ESP_OK;
}
