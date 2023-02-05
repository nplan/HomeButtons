#include "memory.h"
#include "Esp.h"

void memory::log_stack_status(const TaskHandle_t& button_task_h, const TaskHandle_t& display_task_h, const TaskHandle_t& network_task_h, const TaskHandle_t& leds_task_h, const TaskHandle_t& main_task_h)
{
    uint32_t btns_free = uxTaskGetStackHighWaterMark(button_task_h);
    uint32_t disp_free = uxTaskGetStackHighWaterMark(display_task_h);
    uint32_t net_free = uxTaskGetStackHighWaterMark(network_task_h);
    uint32_t leds_free = uxTaskGetStackHighWaterMark(leds_task_h);
    uint32_t main_free = uxTaskGetStackHighWaterMark(main_task_h);
    uint32_t num_tasks = uxTaskGetNumberOfTasks();
    log_d(
        "[DEVICE] free stack: btns %d, disp %d, net %d, leds %d, main "
        "%d, num tasks %d",
        btns_free, disp_free, net_free, leds_free, main_free,
        num_tasks);
    uint32_t esp_free_heap = ESP.getFreeHeap();
    uint32_t esp_min_free_heap = ESP.getMinFreeHeap();
    uint32_t rtos_free_heap = xPortGetFreeHeapSize();
    log_d("[DEVICE] free heap: esp %d, esp min %d, rtos %d",
            esp_free_heap, esp_min_free_heap, rtos_free_heap);
}