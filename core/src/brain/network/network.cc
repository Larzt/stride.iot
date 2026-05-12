#include "network.hpp"

#include "nvs_flash.h"
#include <string>

int Network::_current_station_retries = 0;

Network::Network() : _led(Blackboard::NetLed)
{
  StrideLocator::Register<class Network>(this);
}

void Network::connect()
{
  std::string ssid, password;

  // NVS init
  esp_err_t res = nvs_flash_init();
  if (res == ESP_ERR_NVS_NO_FREE_PAGES || res == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    res = nvs_flash_init();
  }
  ESP_ERROR_CHECK(res);

  // Net init
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  // WiFi init
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  // Event handlers
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, this, NULL));

  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, this, NULL));

  bool has_sta = load_net_credentials(ssid, password);

  wifi_config_t sta_config = {};

  if (has_sta)
  {
    strlcpy((char *)sta_config.sta.ssid, ssid.c_str(), sizeof(sta_config.sta.ssid));
    strlcpy((char *)sta_config.sta.password, password.c_str(), sizeof(sta_config.sta.password));

    esp_netif_create_default_wifi_sta();

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
  }
  else
  {
    StrideLogger::Log(StrideSubsystem::Network, "Fallback a Access Point");
    esp_wifi_disconnect();
    esp_wifi_stop();
    esp_netif_create_default_wifi_ap();
    this->start_access_point();
  }

  ESP_ERROR_CHECK(esp_wifi_start());
}

void Network::start_access_point()
{
  if (_ap_netif == nullptr)
  {
    _ap_netif = esp_netif_create_default_wifi_ap();
  }

  wifi_config_t ap_config = {};

  std::string ssid = Blackboard::AccessSSID;
  std::string pass = Blackboard::AccessPASS;

  strlcpy((char *)ap_config.ap.ssid, ssid.c_str(), sizeof(ap_config.ap.ssid));
  strlcpy((char *)ap_config.ap.password, pass.c_str(), sizeof(ap_config.ap.password));

  ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
  ap_config.ap.max_connection = Blackboard::MaxNetConnections;
  ap_config.ap.channel = 1;

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  esp_netif_ip_info_t ip_info;
  if (esp_netif_get_ip_info(_ap_netif, &ip_info) == ESP_OK)
  {
    char ip_str[16];
    snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&ip_info.ip));
    Blackboard::LocalIpAddress = ip_str;
  }

  Blackboard::CurrentNetworkMode = NetworkMode::Access;
}

bool Network::load_net_credentials(std::string &ssid, std::string &password)
{
  nvs_handle_t handle;
  if (nvs_open("wifi", NVS_READONLY, &handle) != ESP_OK)
  {
    Blackboard::CurrentServerMode.set(ServerMode::Developer);
    return false;
  }

  uint8_t mode_val = 0;
  if (nvs_get_u8(handle, "srv_mode", &mode_val) == ESP_OK)
  {
    Blackboard::CurrentServerMode.set(static_cast<ServerMode>(mode_val));
  }
  else
  {
    Blackboard::CurrentServerMode.set(ServerMode::Developer);
  }

  size_t ssid_len = 0, pass_len = 0;

  if (nvs_get_str(handle, "ssid", NULL, &ssid_len) != ESP_OK ||
      nvs_get_str(handle, "password", NULL, &pass_len) != ESP_OK)
  {
    nvs_close(handle);
    return false;
  }

  char *ssid_buf = new char[ssid_len];
  char *pass_buf = new char[pass_len];

  nvs_get_str(handle, "ssid", ssid_buf, &ssid_len);
  nvs_get_str(handle, "password", pass_buf, &pass_len);

  ssid = ssid_buf;
  password = pass_buf;

  delete[] ssid_buf;
  delete[] pass_buf;

  nvs_close(handle);
  return true;
}

void Network::save_net_credentials(const std::string &ssid, const std::string &password)
{
  nvs_handle_t handle;
  ESP_ERROR_CHECK(nvs_open("wifi", NVS_READWRITE, &handle));

  ESP_ERROR_CHECK(nvs_set_str(handle, "ssid", ssid.c_str()));
  ESP_ERROR_CHECK(nvs_set_str(handle, "password", password.c_str()));

  uint8_t mode = static_cast<uint8_t>(ServerMode::Production);
  ESP_ERROR_CHECK(nvs_set_u8(handle, "srv_mode", mode));

  ESP_ERROR_CHECK(nvs_commit(handle));
  nvs_close(handle);

  Blackboard::CurrentServerMode.set(ServerMode::Production);
}

void Network::event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  Network *self = static_cast<Network *>(arg);

  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
  {
    esp_wifi_connect();
  }

  else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
  {
    self->_led.off();
    Blackboard::CurrentNetworkMode = NetworkMode::Access;

    if (_current_station_retries < Blackboard::MaxConnectionRetries)
    {
      StrideLogger::Log(StrideSubsystem::Network, "Reintentando conexión STA...");
      esp_wifi_connect();
      _current_station_retries++;
    }
    else
    {
      StrideLogger::Log(StrideSubsystem::Network, "Fallback a Access Point");
      self->start_access_point();
    }
  }

  else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
  {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

    char ip_str[16];
    snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&event->ip_info.ip));

    Blackboard::WifiIpAddress = ip_str;

    self->_led.on();

    TimeUtils::initialize();

    StrideLogger::Log(StrideSubsystem::Network, "WiFi IP: %s", Blackboard::WifiIpAddress.c_str());

    Blackboard::CurrentNetworkMode = NetworkMode::Station;

    _current_station_retries = 0;
  }
}
