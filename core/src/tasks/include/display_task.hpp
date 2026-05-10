#pragma once
#include "display.hpp"
#include "freertos/task.h"
#include "stride_button.hpp"
#include "types.hpp"

void display_task(void *pvParameters);
