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
#include "imu_task.hpp"

extern "C" void app_main(void)
{

    Display::Instance().begin();

    spi_sd_init();

    ESP_ERROR_CHECK(i2c_master_init());
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_ERROR_CHECK(imu_init());
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

    xTaskCreatePinnedToCore(
        hear_server_mode_button_task,
        "HearServerModeButton",
        4096,
        NULL,
        5,
        NULL,
        1);

    xTaskCreatePinnedToCore(
        hear_program_selected_file_button_task,
        "HearServerModeButton",
        4096,
        NULL,
        5,
        NULL,
        1);

    xTaskCreatePinnedToCore(
        open_card_task,
        "OpenCard",
        4096,
        NULL,
        5,
        NULL,
        1);

    xTaskCreatePinnedToCore(
        read_card_task,
        "OpenCard",
        4096,
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

    xTaskCreate(
        imu_task,
        "imu_task",
        4096,
        nullptr,
        5,
        &g_imu_task_handle);

    vTaskSuspend(g_imu_task_handle);

    // xTaskCreatePinnedToCore(
    //     expander_task,
    //     "Expand",
    //     4096,
    //     NULL,
    //     5,
    //     NULL,
    //     0);

    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
