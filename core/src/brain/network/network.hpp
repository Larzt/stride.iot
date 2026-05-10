#pragma once
#include <string>
#include "esp_wifi.h"
#include "esp_event.h"

#include "stride_led.hpp"
#include "stride_logger.hpp"
#include "blackboard.hpp"
#include "enums.hpp"


class Network
{
public:
  Network();
  ~Network() {};

  void connect();
  static bool load_net_credentials(std::string &ssid, std::string &pass);
  static void save_net_credentials(const std::string &ssid, const std::string &pass);


private:
  StrideLed _led;
  static int _current_station_retries;

  static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
};
