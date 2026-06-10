#include "timer.hpp"

#include "esp_timer.h"

#include <time.h>
#include <sys/time.h>

bool TimeUtils::_initialized = false;

void TimeUtils::initialize()
{
  if (_initialized)
    return;

  setenv(Blackboard::TimeZone, "WET0WEST,M3.5.0/1,M10.5.0", 1);
  tzset();

  // Sin WiFi ni RTC externo el ESP32 no puede conocer la hora real al
  // arrancar, así que sembramos el reloj con la fecha/hora de compilación
  // (cuando se flasheó el firmware) como punto de partida aproximado.
  struct tm tm = {};
  if (strptime(__DATE__ " " __TIME__, "%b %d %Y %H:%M:%S", &tm) != nullptr)
  {
    tm.tm_isdst = -1;
    struct timeval tv = {.tv_sec = mktime(&tm), .tv_usec = 0};
    settimeofday(&tv, nullptr);

    StrideLogger::Log(StrideSubsystem::Utils, "Reloj sembrado con hora de compilación");
  }
  else
  {
    StrideLogger::Error(StrideSubsystem::Utils, "No se pudo sembrar el reloj");
  }

  _initialized = true;
}

std::string TimeUtils::get_timestamp()
{
  time_t now;
  struct tm timeinfo;

  time(&now);
  localtime_r(&now, &timeinfo);

  char date_buffer[32];
  strftime(
      date_buffer,
      sizeof(date_buffer),
      "%d/%m/%Y - %H:%M:%S",
      &timeinfo);

  // El reloj de pared vuelve a la hora de compilación en cada arranque en
  // frío, así que añadimos el tiempo de actividad (monótono) para poder
  // ordenar y medir eventos aunque la fecha se haya reiniciado.
  int64_t uptime_s = esp_timer_get_time() / 1000000;
  int hours = uptime_s / 3600;
  int minutes = (uptime_s % 3600) / 60;
  int seconds = uptime_s % 60;

  char buffer[64];
  snprintf(
      buffer,
      sizeof(buffer),
      "%s [+%02d:%02d:%02d]",
      date_buffer,
      hours,
      minutes,
      seconds);

  return std::string(buffer);
}
