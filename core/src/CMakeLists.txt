file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cc" "*.cpp" "*.c")

message(STATUS "SOURCES: ${SOURCES}")

idf_component_register(
  SRCS ${SOURCES}

  INCLUDE_DIRS
  "."
  "types"
  "tasks"
  "utils"
  "utils/timer"
  "brain/apps"
  "utils/parse"
  "utils/sink"
  "utils/hexer"
  "tasks/include"
  "handler"
  "handler/include"
  "brain"
  "brain/expander"
  "brain/interpreter"
  "brain/network"
  "brain/server"
  "brain/bus"
  "brain/display"
  "brain/display/include"

  REQUIRES logger io observer lang locator
  REQUIRES esp_wifi esp_netif esp_event nvs_flash esp_http_server lwip esp_timer

  PRIV_REQUIRES esp_driver_gpio driver fatfs sdmmc spi_flash LovyanGFX
)
