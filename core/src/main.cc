#include "stride_logger.hpp"
#include "network.hpp"
#include "server.hpp"

#include "bus.hpp"
#include "display.hpp"
#include "server_mode_task.hpp"
#include "select_task.hpp"
#include "card_task.hpp"
#include "display_task.hpp"
#include "expander_task.hpp"
#include "app_manager.hpp"

extern "C" void app_main(void)
{
    AppManager::Instance();

    Display::Instance().begin();

    spi_sd_init();

    ESP_ERROR_CHECK(i2c_master_init());
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_ERROR_CHECK(expander_init());

    class Network network;
    network.connect();

    class Server server;
    server.start_server();

    StrideLed server_mode_led(GPIO_NUM_26, true);
    Blackboard::CurrentServerMode.subscribe([&server_mode_led](const auto &mode)
                                            {
        StrideLogger::Log(StrideSubsystem::Server, "Server changed mode");
        server_mode_led.toggle(); });

    // Core 0: system tasks, monitoring, lightweight
    // Core 0 also runs the WiFi stack and HTTP server handlers by default.
    // Tasks here are mostly sleeping or event-driven and can safely log or
    // update Blackboard observables to notify the rest of the system.

    xTaskCreatePinnedToCore(
        hear_server_mode_button_task,
        "ServerModeBtn",
        4096,
        NULL,
        5,
        NULL,
        0);

    xTaskCreatePinnedToCore(
        open_card_task,
        "CardMonitor",
        6144,
        NULL,
        5,
        NULL,
        0);

    // Core 1: computation and user-facing tasks
    // display_task owns the SPI2 bus (TFT).
    // hear_program_selected_file_button_task calls Display::transition_to()
    // directly, so it must share a core with display_task.
    // read_card_task runs the DSL interpreter (CPU-intensive, I2C, SD file I/O).

    xTaskCreatePinnedToCore(
        hear_program_selected_file_button_task,
        "ProgramSelectBtn",
        4096,
        NULL,
        5,
        NULL,
        1);

    xTaskCreatePinnedToCore(
        read_card_task,
        "ScriptRunner",
        8192,
        NULL,
        5,
        &sdReadTaskHandle,
        1);

    xTaskCreatePinnedToCore(
        display_task,
        "DisplayTFT",
        4096,
        NULL,
        5,
        NULL,
        1);

    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
