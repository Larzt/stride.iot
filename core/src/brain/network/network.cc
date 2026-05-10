#include "network.hpp"

#include "nvs_flash.h"
#include <string>

int Network::_current_station_retries = 0;

Network::Network() : _led(Blackboard::NetLed)
{
}

void Network::connect()
{
  std::string ssid, password;

  // NVS Initialization
  esp_err_t res = nvs_flash_init();
  if (res == ESP_ERR_NVS_NO_FREE_PAGES || res == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    res = nvs_flash_init();
  }
  ESP_ERROR_CHECK(res);

  // Network stack Initialization
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  esp_netif_create_default_wifi_sta();
  esp_netif_create_default_wifi_ap();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  // Events register
  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, this, NULL));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, this, NULL));

  // Load STA saved credentials
  wifi_config_t sta_config = {};
  bool has_sta = false;
  if (load_net_credentials(ssid, password))
  {
    StrideLogger::Log(StrideSubsystem::Network, "Loading credentials: %s", ssid.c_str());
    strlcpy((char *)sta_config.sta.ssid, ssid.c_str(), sizeof(sta_config.sta.ssid));
    strlcpy((char *)sta_config.sta.password, password.c_str(), sizeof(sta_config.sta.password));
    has_sta = true;
  }

  // Load AP saved credentials
  wifi_config_t ap_config = {};
  strlcpy((char *)ap_config.ap.ssid, Blackboard::AccessSSID.c_str(), sizeof(ap_config.ap.ssid));
  strlcpy((char *)ap_config.ap.password, Blackboard::AccessPASS.c_str(), sizeof(ap_config.ap.password));
  ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
  ap_config.ap.max_connection = Blackboard::MaxNetConnections;
  ap_config.ap.channel = 1;

  // Network start
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));

  if (has_sta)
  {
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
  }

  ESP_ERROR_CHECK(esp_wifi_start());

  // Save local ip_address
  esp_netif_ip_info_t ip_info;
  esp_netif_t *ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
  if (ap_netif != nullptr && esp_netif_get_ip_info(ap_netif, &ip_info) == ESP_OK)
  {
    char ip_str[16];
    snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&ip_info.ip));
    Blackboard::LocalIpAddress = ip_str;
  }

  // if (has_sta)
  // {
  //   ESP_ERROR_CHECK(esp_wifi_connect());
  // }

  Blackboard::CurrentNetworkMode = has_sta ? NetworkMode::Station : NetworkMode::Access;
}

bool Network::load_net_credentials(std::string &ssid, std::string &password)
{
  nvs_handle_t handle;
  if (nvs_open("wifi", NVS_READONLY, &handle) != ESP_OK)
  {
    return false;
  }

  size_t ssid_length = 0, password_length = 0;
  if (nvs_get_str(handle, "ssid", NULL, &ssid_length) != ESP_OK)
  {
    nvs_close(handle);
    return false;
  }

  if (nvs_get_str(handle, "password", NULL, &password_length) != ESP_OK)
  {
    nvs_close(handle);
    return false;
  }

  char *ssid_buffer = new char[ssid_length];
  char *password_buffer = new char[password_length];

  nvs_get_str(handle, "ssid", ssid_buffer, &ssid_length);
  nvs_get_str(handle, "password", password_buffer, &password_length);

  ssid = ssid_buffer;
  password = password_buffer;

  delete[] ssid_buffer;
  delete[] password_buffer;
  nvs_close(handle);

  return true;
}

void Network::save_net_credentials(const std::string &ssid, const std::string &password)
{
  nvs_handle_t handle;
  ESP_ERROR_CHECK(nvs_open("wifi", NVS_READWRITE, &handle));
  ESP_ERROR_CHECK(nvs_set_str(handle, "ssid", ssid.c_str()));
  ESP_ERROR_CHECK(nvs_set_str(handle, "password", password.c_str()));
  ESP_ERROR_CHECK(nvs_commit(handle));
  nvs_close(handle);
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
    if (_current_station_retries <= Blackboard::MaxConnectionRetries)
    {
      StrideLogger::Log(StrideSubsystem::Network, "Station reconnection");
      esp_wifi_connect();
      _current_station_retries++;
    }
  }
  else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
  {
    ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
    char ip_str[16];
    snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&event->ip_info.ip));

    Blackboard::WifiIpAddress = ip_str;

    self->_led.on();

    StrideLogger::Log(StrideSubsystem::Network, "Local IP: %s", Blackboard::LocalIpAddress.c_str());
    StrideLogger::Log(StrideSubsystem::Network, "WiFi Assigned IP: %s", Blackboard::WifiIpAddress.c_str());

    Blackboard::CurrentNetworkMode = NetworkMode::Station;
    _current_station_retries = 0;
  }
}
