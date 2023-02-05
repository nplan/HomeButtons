#ifndef HOMEBUTTONS_MEMORY_H
#define HOMEBUTTONS_MEMORY_H

#include "FreeRTOS.h"
#include "task.h"

namespace memory {
    void log_stack_status(const TaskHandle_t& button_task_h, const TaskHandle_t& display_task_h, const TaskHandle_t& network_task_h, const TaskHandle_t& leds_task_h, const TaskHandle_t& main_task_h);
}

#endif // HOMEBUTTONS_MEMORY_H