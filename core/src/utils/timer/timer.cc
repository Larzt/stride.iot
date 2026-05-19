#include "timer.hpp"

#include "esp_sntp.h"

#include <time.h>

bool TimeUtils::_initialized = false;

void TimeUtils::initialize()
{
  if (_initialized)
    return;

  setenv(Blackboard::TimeZone, "WET0WEST,M3.5.0/1,M10.5.0", 1);
  tzset();

  esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);

  esp_sntp_setservername(0, "pool.ntp.org");
  esp_sntp_setservername(1, "time.nist.gov");

  esp_sntp_init();

  time_t now = 0;
  struct tm timeinfo = {};

  int retries = 0;
  const int max_retries = 15;

  while (timeinfo.tm_year < (2024 - 1900) && retries < max_retries)
  {
    time(&now);
    localtime_r(&now, &timeinfo);

    vTaskDelay(pdMS_TO_TICKS(1000));

    retries++;
  }

  if (retries < max_retries)
  {
    StrideLogger::Log(StrideSubsystem::Utils, "NTP sincronizado");
    _initialized = true;
  }
  else
  {
    StrideLogger::Error(StrideSubsystem::Utils, "Error sincronizando NTP");
  }
}

std::string TimeUtils::get_timestamp()
{
  time_t now;
  struct tm timeinfo;

  time(&now);
  localtime_r(&now, &timeinfo);

  char buffer[32];

  strftime(
      buffer,
      sizeof(buffer),
      "%d/%m/%Y - %H:%M",
      &timeinfo);

  return std::string(buffer);
}
